package tsbase

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"

	"github.com/kannachi323/misty/proxy/core/utils"
)

type TSConfig struct {
	DeviceID string `json:"device_id"`
	ServerID string `json:"server_id"`
	BaseName string `json:"base_name"`

}

func GetConfigSecretPath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		log.Println("Could not determine user home directory:", err)
		return ""
	}
	secretPath := filepath.Join(home, "misty", "minidfs", "tailscale", "secret.txt")

	return secretPath
}

func GetConfigPath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		log.Println("Could not determine user home directory:", err)
		return ""
	}
	configPath := filepath.Join(home, "misty", "minidfs", "tailscale", "config.json")

	return configPath
}	

func (config *TSConfig) GetHashedBaseName() string {
	secretPath := GetConfigSecretPath()
	if secretPath == "" {
		return config.BaseName
	}

	secret, err := os.ReadFile(secretPath)
	if err != nil {
		fmt.Printf("error reading secret file: %w", err)
		return ""
	}
	hash := utils.GenerateShortHash(secret, config.BaseName)
	serverHostName := fmt.Sprintf("%s-%s", config.BaseName, hash)

	return serverHostName
}

func LoadConfig(path string) (*TSConfig, error) {
	file, err := os.Open(path)
	if err != nil { 
		return nil, fmt.Errorf("could not open config file: %w", err)
	}
	defer file.Close()

	var config TSConfig
	decoder := json.NewDecoder(file)
	if err := decoder.Decode(&config); err != nil {
		return nil, fmt.Errorf("could not decode config file: %w", err)
	}

	return &config, nil
}

