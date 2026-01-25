package ms

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/url"
	"strings"
)

// CreateUploadSessionRequest is the JSON body for creating an upload session
type CreateUploadSessionRequest struct {
	Path     string `json:"path"`     // folder path, e.g. "Documents" or "Documents/subdir", empty = root
	FileName string `json:"fileName"` // name of the file to upload
}

// UploadSessionResponse holds the createUploadSession response from Microsoft Graph
type UploadSessionResponse struct {
	UploadURL          string   `json:"uploadUrl"`
	ExpirationDateTime string   `json:"expirationDateTime,omitempty"`
	NextExpectedRanges []string `json:"nextExpectedRanges,omitempty"`
}

// graphUploadSessionResp matches the Graph API createUploadSession response
type graphUploadSessionResp struct {
	UploadURL          string   `json:"uploadUrl"`
	ExpirationDateTime string   `json:"expirationDateTime"`
	NextExpectedRanges []string `json:"nextExpectedRanges"`
}

const graphBase = "https://graph.microsoft.com/v1.0"

func CreateUploadSession() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
			return
		}

		// 1. Bearer token (from OAuth callback, client sends it)
		auth := r.Header.Get("Authorization")
		if auth == "" || !strings.HasPrefix(strings.ToLower(auth), "bearer ") {
			http.Error(w, "Authorization: Bearer <token> required", http.StatusUnauthorized)
			return
		}
		token := strings.TrimSpace(strings.TrimPrefix(auth, "Bearer "))
		if token == "" {
			http.Error(w, "Authorization: Bearer <token> required", http.StatusUnauthorized)
			return
		}

		// 2. Parse request body
		var req CreateUploadSessionRequest
		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "Invalid JSON body: "+err.Error(), http.StatusBadRequest)
			return
		}
		req.Path = strings.Trim(req.Path, "/")
		if req.FileName == "" {
			http.Error(w, "fileName is required", http.StatusBadRequest)
			return
		}

		// 3. Build Graph path: root:/{path}/fileName: (encode each segment)
		itemPath := req.FileName
		if req.Path != "" {
			itemPath = req.Path + "/" + req.FileName
		}
		parts := strings.Split(itemPath, "/")
		for i, p := range parts {
			parts[i] = url.PathEscape(p)
		}
		encodedPath := strings.Join(parts, "/")
		graphURL := graphBase + "/me/drive/root:/" + encodedPath + ":/createUploadSession"

		// 4. Request body for createUploadSession
		body := map[string]interface{}{
			"item": map[string]string{
				"@microsoft.graph.conflictBehavior": "rename",
				"name": req.FileName,
			},
		}
		bodyBytes, err := json.Marshal(body)
		if err != nil {
			http.Error(w, "Failed to build request: "+err.Error(), http.StatusInternalServerError)
			return
		}

		// 5. Call Microsoft Graph
		httpReq, err := http.NewRequestWithContext(r.Context(), http.MethodPost, graphURL, bytes.NewReader(bodyBytes))
		if err != nil {
			http.Error(w, "Failed to create request: "+err.Error(), http.StatusInternalServerError)
			return
		}
		httpReq.Header.Set("Authorization", "Bearer "+token)
		httpReq.Header.Set("Content-Type", "application/json")

		resp, err := http.DefaultClient.Do(httpReq)
		if err != nil {
			http.Error(w, "Graph request failed: "+err.Error(), http.StatusInternalServerError)
			return
		}
		defer resp.Body.Close()

		if resp.StatusCode != http.StatusOK {
			var errBody struct {
				Error struct {
					Code    string `json:"code"`
					Message string `json:"message"`
				} `json:"error"`
			}
			_ = json.NewDecoder(resp.Body).Decode(&errBody)
			msg := errBody.Error.Message
			if msg == "" {
				msg = resp.Status
			}
			http.Error(w, "Graph createUploadSession failed: "+msg, resp.StatusCode)
			return
		}

		var graphResp graphUploadSessionResp
		if err := json.NewDecoder(resp.Body).Decode(&graphResp); err != nil {
			http.Error(w, "Failed to decode Graph response: "+err.Error(), http.StatusInternalServerError)
			return
		}

		out := UploadSessionResponse{
			UploadURL:          graphResp.UploadURL,
			ExpirationDateTime: graphResp.ExpirationDateTime,
			NextExpectedRanges: graphResp.NextExpectedRanges,
		}
		w.Header().Set("Content-Type", "application/json")
		enc := json.NewEncoder(w)
		enc.SetEscapeHTML(false)
		_ = enc.Encode(out)
	}
}
