package ms

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/url"

	"github.com/kannachi323/misty/proxy/core/ms"
	"github.com/kannachi323/misty/proxy/db"
)

func GetMSTokens(db *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		userID := r.URL.Query().Get("user_id")
		if userID == "" {
			http.Error(w, "user_id parameter required", http.StatusBadRequest)
			return
		}
		
		tokens, err := db.GetMSTokens(userID)
		if err != nil {
			http.Error(w, "failed to get tokens", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(tokens)
	}
}

func UpdateMSToken(db *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		accessToken := r.URL.Query().Get("access_token")
		refreshToken := r.URL.Query().Get("refresh_token")
		userID := r.URL.Query().Get("user_id")
		if accessToken == "" || refreshToken == "" || userID == "" {
			http.Error(w, "access_token, refresh_token, and user_id are required", http.StatusBadRequest)
			return
		}

		// Fetch Microsoft user profile from Graph /me before storing
		profile, err := FetchMSUserProfile(accessToken)
		if err != nil {
			ServeMSAuthHTML(w, false)
			return
		}

		err = db.StoreMSToken(userID, profile.ID, accessToken, refreshToken, profile.DisplayName, profile.Email())
		if err != nil {
			ServeMSAuthHTML(w, false)
			return
		}

		ServeMSAuthHTML(w, true)
	}
}

func DeleteMSToken(db *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodDelete {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		userID := r.URL.Query().Get("user_id")
		msUserID := r.URL.Query().Get("ms_user_id")
		if userID == "" || msUserID == "" {
			http.Error(w, "user_id and ms_user_id parameters required", http.StatusBadRequest)
			return
		}

		if db == nil {
			http.Error(w, "Database not available", http.StatusInternalServerError)
			return
		}

		if err := db.DeleteMSToken(userID, msUserID); err != nil {
			fmt.Printf("[MS Token] Failed to delete token: %v\n", err)
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}

		fmt.Printf("[MS Token] Deleted token for user %s, ms_user %s\n", userID, msUserID)
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]interface{}{
			"success": true,
			"message": "Token deleted",
		})
	}
}

type RefreshTokenResponse struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
	ExpiresIn    int    `json:"expires_in"`
}

func RefreshMSToken(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		userID := r.URL.Query().Get("user_id")
		msUserID := r.URL.Query().Get("ms_user_id")
		if userID == "" || msUserID == "" {
			http.Error(w, "user_id and ms_user_id parameters required", http.StatusBadRequest)
			return
		}

		tokens, err := database.GetMSTokens(userID)
		if err != nil {
			http.Error(w, "Failed to get tokens", http.StatusInternalServerError)
			return
		}

		var existingToken *db.MSTokenRecord
		for _, t := range tokens {
			if t.MsUserID == msUserID {
				existingToken = &t
				break
			}
		}
		if existingToken == nil || existingToken.RefreshToken == "" {
			http.Error(w, "No refresh token found for this user", http.StatusNotFound)
			return
		}

		config := ms.GetConfig()
		if config == nil {
			http.Error(w, "Failed to get MS config", http.StatusInternalServerError)
			return
		}

		resp, err := http.PostForm("https://login.microsoftonline.com/common/oauth2/v2.0/token", url.Values{
			"client_id":     {config.ClientID},
			"client_secret": {config.ClientSecret},
			"refresh_token": {existingToken.RefreshToken},
			"grant_type":    {"refresh_token"},
			"scope":         {config.GetScopesString()},
		})
		if err != nil {
			http.Error(w, "Failed to refresh token", http.StatusInternalServerError)
			return
		}
		defer resp.Body.Close()

		var tokenResp map[string]interface{}
		if err := json.NewDecoder(resp.Body).Decode(&tokenResp); err != nil {
			http.Error(w, "Failed to parse token response", http.StatusInternalServerError)
			return
		}

		if errMsg, ok := tokenResp["error"]; ok {
			errDesc, _ := tokenResp["error_description"].(string)
			http.Error(w, fmt.Sprintf("Token refresh failed: %v - %s", errMsg, errDesc), http.StatusUnauthorized)
			return
		}

		newAccessToken, _ := tokenResp["access_token"].(string)
		newRefreshToken, _ := tokenResp["refresh_token"].(string)
		expiresIn, _ := tokenResp["expires_in"].(float64)

		if newAccessToken == "" {
			http.Error(w, "No access token in response", http.StatusInternalServerError)
			return
		}

		if err := database.StoreMSToken(userID, msUserID, newAccessToken, newRefreshToken, existingToken.DisplayName, existingToken.Email); err != nil {
			http.Error(w, "Failed to store new tokens", http.StatusInternalServerError)
			return
		}

		fmt.Printf("[MS Token] Refreshed token for user %s, ms_user %s\n", userID, msUserID)
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(RefreshTokenResponse{
			AccessToken:  newAccessToken,
			RefreshToken: newRefreshToken,
			ExpiresIn:    int(expiresIn),
		})
	}
}