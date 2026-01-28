package db

import (
	"log"

	"github.com/google/uuid"
	"golang.org/x/crypto/bcrypt"
)

func (db *Database) InsertUser(name, email, password string) error {
    id := uuid.New().String()
    
    hashedPassword, _ := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)

    _, err := db.Conn.Exec(`
        INSERT INTO users (id, name, email, password) 
        VALUES (?, ?, ?, ?)`, 
        id, name, email, hashedPassword, 0,
    )
    if err != nil {
        log.Println("Failed to create user:", err)
    }
    return err
}

func (db *Database) GetUser(email, password string) error {
    var storedHashedPassword string
    err := db.Conn.QueryRow(`
        SELECT password FROM users WHERE email = ?`, 
        email,
    ).Scan(&storedHashedPassword)
    if err != nil {
        log.Println("Failed to get user:", err)
        return err
    }

    err = bcrypt.CompareHashAndPassword([]byte(storedHashedPassword), []byte(password))
    if err != nil {
        log.Println("Invalid password:", err)
        return err
    }

    return nil
}