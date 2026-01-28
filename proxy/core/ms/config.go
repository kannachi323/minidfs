package ms

import (
	"log"
	"os"
	"sync"

	"github.com/joho/godotenv"
)

type MSConfig struct {
	ClientID     string
	ClientSecret string
	RedirectURI  string
	Authority    string
	Scopes       []string
}

var (
	config     *MSConfig
	configOnce sync.Once
)

func GetConfig() *MSConfig {
	configOnce.Do(func() {
		if err := godotenv.Load(); err != nil {
			log.Println("No .env file found, relying on system environment variables")
		}

		config = &MSConfig{
			ClientID:     os.Getenv("MS_CLIENT_ID"),
			ClientSecret: os.Getenv("MS_CLIENT_SECRET"),
			RedirectURI:  os.Getenv("MS_REDIRECT_URI"),
			Authority:    "https://login.microsoftonline.com/common",
			Scopes: []string{
				"https://graph.microsoft.com/User.Read",
				"https://graph.microsoft.com/Files.ReadWrite.All",
				"offline_access",
			},
		}
	})
	return config
}

func (c *MSConfig) GetScopesString() string {
	result := ""
	for i, s := range c.Scopes {
		if i > 0 {
			result += " "
		}
		result += s
	}
	return result
}