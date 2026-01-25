package ms

import (
	"fmt"
	"log"
	"os"
	"sync"

	"github.com/AzureAD/microsoft-authentication-library-for-go/apps/confidential"
	"github.com/joho/godotenv"
)

var (
	msalClientOnce sync.Once
	msalClient     confidential.Client
	msalClientErr  error

	// Configuration
	authority = "https://login.microsoftonline.com/consumers"
	scopes = []string{
		"Files.ReadWrite", // Covers ALL file operations
		"User.Read",           // For basic login info                                     // For the ID Token
	}
)

// GetMSALClient returns the singleton confidential client
func GetMSALClient() (confidential.Client, error) {
	msalClientOnce.Do(func() {
		if err := godotenv.Load(); err != nil {
			log.Println("No .env file found, relying on system environment variables")
		}
		clientID := os.Getenv("MS_CLIENT_ID")
		secret := os.Getenv("MS_CLIENT_SECRET")

		if clientID == "" || secret == "" {
			msalClientErr = fmt.Errorf("MS_CLIENT_ID or MS_CLIENT_SECRET not set")
			return
		}

		cred, err := confidential.NewCredFromSecret(secret)
		if err != nil {
			msalClientErr = err
			return
		}

		msalClient, msalClientErr = confidential.New(
			authority,
			clientID,
			cred,
		)
	})
	return msalClient, msalClientErr
}

// Helper getters
func GetRedirectURI() string { return os.Getenv("MS_REDIRECT_URI") }
func GetScopes() []string    { return scopes }
func GetClientID() string    { return os.Getenv("MS_CLIENT_ID") }