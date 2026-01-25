package ms

import (
	"encoding/json"
	"net/http"

	"github.com/kannachi323/misty/proxy/core/ms"
)

type OAuthLoginResponse struct {
	AuthURL string `json:"auth_url"`
}

func GetOAuthLogin() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		client, err := ms.GetMSALClient()
		if err != nil {
			http.Error(w, "Client init failed: "+err.Error(), http.StatusInternalServerError)
			return	
		}

		authURL, err := client.AuthCodeURL(
			r.Context(),
			ms.GetClientID(),
			ms.GetRedirectURI(),
			ms.GetScopes(),
	
		)
		if err != nil {
			http.Error(w, "Failed to generate URL: "+err.Error(), http.StatusInternalServerError)
			return
		}
		authURL += "&prompt=consent"


		response := OAuthLoginResponse{
			AuthURL: authURL,
		}

		w.Header().Set("Content-Type", "application/json")
		enc := json.NewEncoder(w)
        enc.SetEscapeHTML(false) 
        enc.Encode(response)
	}
}

func OAuthCallback() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		code := r.URL.Query().Get("code")
		if code == "" {
			http.Error(w, "Code parameter is required", http.StatusBadRequest)
			return
		}
		client, err := ms.GetMSALClient()
		if err != nil {
			http.Error(w, "Client init failed: "+err.Error(), http.StatusInternalServerError)
			return
		}
		result, err := client.AcquireTokenByAuthCode(r.Context(), code, ms.GetRedirectURI(), ms.GetScopes())
		if err != nil {
			http.Error(w, "Failed to acquire token: "+err.Error(), http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		enc := json.NewEncoder(w)
        enc.SetEscapeHTML(false) 
        enc.Encode(result)
	}
}