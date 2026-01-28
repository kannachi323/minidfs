package ms

import (
	"encoding/base64"
	"encoding/json"
	"log"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"strings"

	"github.com/kannachi323/misty/proxy/core/ms"
	"github.com/kannachi323/misty/proxy/db"
)

type OAuthLoginResponse struct {
	AuthURL string `json:"auth_url"`
}

func GetOAuthLogin() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		config := ms.GetConfig()
		if config == nil {
			http.Error(w, "Failed to get MS config", http.StatusInternalServerError)
			return
		}

		userID := r.URL.Query().Get("user_id")
		if userID == "" {
			http.Error(w, "user_id parameter is required", http.StatusBadRequest)
			return
		}
		
		params := url.Values{
			"client_id":     {config.ClientID},
			"redirect_uri":  {config.RedirectURI},
			"response_type": {"code"},
			"scope":         {config.GetScopesString()},
			"prompt":        {"consent"},
		}

		// Encode user_id in the state parameter - Microsoft will return this in the callback
		params.Set("state", base64.URLEncoding.EncodeToString([]byte(userID)))
	

		authURL := "https://login.microsoftonline.com/common/oauth2/v2.0/authorize?" + params.Encode()

		response := OAuthLoginResponse{AuthURL: authURL}
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(response)
	}
}

func OAuthCallback(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		config := ms.GetConfig()
		code := r.URL.Query().Get("code")
		state := r.URL.Query().Get("state")

		// Decode user_id from state parameter (Microsoft returns state in callback)
		var userID string
		if state != "" {
			decoded, err := base64.URLEncoding.DecodeString(state)
			if err == nil {
				userID = string(decoded)
			}
		}

		if code == "" || userID == "" {
			log.Printf("code or user_id is empty - code: %s, userID: %s", code, userID)
			ServeMSAuthHTML(w, false)
			return
		}

		resp, err := http.PostForm("https://login.microsoftonline.com/common/oauth2/v2.0/token", url.Values{
			"client_id":     {config.ClientID},
			"client_secret": {config.ClientSecret},
			"code":          {code},
			"redirect_uri":  {config.RedirectURI},
			"grant_type":    {"authorization_code"},
			"scope":         {config.GetScopesString()},
		})
		if err != nil {
			ServeMSAuthHTML(w, false)
			return
		}
		defer resp.Body.Close()

		var tokenResp map[string]interface{}
		if err := json.NewDecoder(resp.Body).Decode(&tokenResp); err != nil {
			ServeMSAuthHTML(w, false);
			return;
		}
		if _, ok := tokenResp["error"]; ok {
			ServeMSAuthHTML(w, false);
			return;
		}

		var refreshToken, accessToken string
		if token, ok := tokenResp["access_token"].(string); ok && token != "" {
			accessToken = token
		}
		if token, ok := tokenResp["refresh_token"].(string); ok && token != "" {
			refreshToken = token
		}

		query := url.Values{}
		query.Set("user_id", userID)
		query.Set("access_token", accessToken)
		query.Set("refresh_token", refreshToken)
		redirectURL := "/api/ms/callback/token?" + query.Encode()

		http.Redirect(w, r, redirectURL, http.StatusFound)
	}
}


func ServeMSAuthHTML(w http.ResponseWriter, success bool) {
	// Read the redirect.html file
	workDir, _ := os.Getwd()
	staticDir := filepath.Join(workDir, "static")
	htmlPath := filepath.Join(staticDir, "redirect.html")

	htmlContent, err := os.ReadFile(htmlPath)
	if err != nil {
		http.Error(w, "Failed to read redirect.html", http.StatusInternalServerError)
		return
	}
	if success == false {
		htmlContent = []byte(strings.Replace(string(htmlContent), "{{ success }}", "false", 1))
	} else {
		htmlContent = []byte(strings.Replace(string(htmlContent), "{{ success }}", "true", 1))
	}

	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Write(htmlContent)
}