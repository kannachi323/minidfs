package db

import (
	"fmt"
)

type MSTokenRecord struct {
	UserID       string `json:"user_id"`
	MsUserID     string `json:"ms_user_id"`
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
	DisplayName  string `json:"display_name"`
	Email        string `json:"email"`
}

// StoreMSToken upserts a token by (user_id, ms_user_id). Use ms_user_id from Graph /me to allow multiple MS accounts per user.
func (db *Database) StoreMSToken(userID, msUserID, accessToken, refreshToken, displayName, email string) error {
	if msUserID == "" {
		return fmt.Errorf("ms_user_id is required")
	}
	// Upsert: insert or replace (SQLite)
	query := `INSERT INTO ms_tokens (user_id, ms_user_id, access_token, refresh_token, display_name, email)
		VALUES (?, ?, ?, ?, ?, ?)
		ON CONFLICT (user_id, ms_user_id) DO UPDATE SET
			access_token = excluded.access_token,
			refresh_token = excluded.refresh_token,
			display_name = excluded.display_name,
			email = excluded.email`
	_, err := db.Conn.Exec(query, userID, msUserID, accessToken, refreshToken, displayName, email)
	if err != nil {
		return fmt.Errorf("failed to store MS token: %w", err)
	}
	return nil
}

// GetMSTokens returns all tokens for a specific user_id
func (db *Database) GetMSTokens(userID string) ([]MSTokenRecord, error) {
	query := `SELECT user_id, ms_user_id, access_token, refresh_token, COALESCE(display_name, ''), COALESCE(email, '') FROM ms_tokens WHERE user_id = ?`
	rows, err := db.Conn.Query(query, userID)
	if err != nil {
		return nil, fmt.Errorf("failed to get MS tokens: %w", err)
	}
	defer rows.Close()

	var tokens []MSTokenRecord
	for rows.Next() {
		var t MSTokenRecord
		if err := rows.Scan(&t.UserID, &t.MsUserID, &t.AccessToken, &t.RefreshToken, &t.DisplayName, &t.Email); err != nil {
			return nil, fmt.Errorf("failed to scan MS token: %w", err)
		}
		tokens = append(tokens, t)
	}
	if err = rows.Err(); err != nil {
		return nil, fmt.Errorf("error iterating tokens: %w", err)
	}
	return tokens, nil
}

// DeleteMSToken deletes the token for (user_id, ms_user_id).
func (db *Database) DeleteMSToken(userID, msUserID string) error {
	query := `DELETE FROM ms_tokens WHERE user_id = ? AND ms_user_id = ?`
	result, err := db.Conn.Exec(query, userID, msUserID)
	if err != nil {
		return fmt.Errorf("failed to delete MS token: %w", err)
	}
	rowsAffected, _ := result.RowsAffected()
	if rowsAffected == 0 {
		return fmt.Errorf("token not found")
	}
	return nil
}
