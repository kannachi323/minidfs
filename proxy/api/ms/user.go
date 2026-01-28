package ms

import (
	"encoding/json"
	"fmt"
	"net/http"
)

type MSUserProfile struct {
	ID                string `json:"id"`
	DisplayName       string `json:"displayName"`
	Mail              string `json:"mail"`
	UserPrincipalName string `json:"userPrincipalName"`
}

// Email returns mail if available, otherwise userPrincipalName
func (p MSUserProfile) Email() string {
	if p.Mail != "" {
		return p.Mail
	}
	return p.UserPrincipalName
}

func FetchMSUserProfile(accessToken string) (*MSUserProfile, error) {
	graphBase := "https://graph.microsoft.com/v1.0"
	req, err := http.NewRequest(http.MethodGet, graphBase+"/me", nil)
	if err != nil {
		return nil, fmt.Errorf("create request: %w", err)
	}
	req.Header.Set("Authorization", "Bearer "+accessToken)
	req.Header.Set("Accept", "application/json")

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, fmt.Errorf("graph request: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode == 401 {
		return nil, fmt.Errorf("token invalid or expired")
	}
	if resp.StatusCode < 200 || resp.StatusCode >= 300 {
		return nil, fmt.Errorf("graph returned %d", resp.StatusCode)
	}

	var profile MSUserProfile
	if err := json.NewDecoder(resp.Body).Decode(&profile); err != nil {
		return nil, fmt.Errorf("decode /me: %w", err)
	}
	if profile.ID == "" {
		return nil, fmt.Errorf("missing id in /me response")
	}
	return &profile, nil
}

// FetchMSUserID is kept for backwards compatibility
func FetchMSUserID(accessToken string) (string, error) {
	profile, err := FetchMSUserProfile(accessToken)
	if err != nil {
		return "", err
	}
	return profile.ID, nil
}
