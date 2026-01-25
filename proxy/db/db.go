package db

import (
	"database/sql"
	"fmt"
	"log"
	"os"
	"path/filepath"

	_ "modernc.org/sqlite"
)


type Database struct {
	Conn *sql.DB
}

func (db *Database) GetDatabasePath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		log.Println("Could not determine user home directory:", err)
		return ""
	}
	databasePath := filepath.Join(home, "misty", "minidfs", "db", "data.db")

	return databasePath
}

func (db *Database) StartDatabase() error {
	databasePath := db.GetDatabasePath()
	if databasePath == "" {
		return fmt.Errorf("invalid database path")
	}

	conn, err := sql.Open("sqlite", databasePath)
	if err != nil {
		log.Println("Failed to open database: ", err)
		return err
	}

	if err := conn.Ping(); err != nil {
		log.Println("Failed to connect to database: ", err)
		return err
	}

	db.Conn = conn

	return nil
}

func (db *Database) Stop() {
	if db.Conn != nil {
		db.Conn.Close()
	}
}