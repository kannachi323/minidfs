package workspaces

import (
	"database/sql"
	"errors"
	"log"

	"github.com/google/uuid"
)

type Workspace struct {
	WorkspaceID   string `json:"workspace_id"`
	WorkspaceName string `json:"workspace_name"`
	MountPath     string `json:"mount_path"`
}

// CreateWorkspace inserts a new workspace
func CreateWorkspace(db *sql.DB, name, mountPoint string) (*Workspace, error) {
	workspaceID := uuid.New().String()

	_, err := db.Exec(`
		INSERT INTO workspaces (workspace_id, workspace_name, mount_path)
		VALUES (?, ?, ?)
	`, workspaceID, name, mountPoint)

	if err != nil {
		log.Printf("Failed to create workspace: %v", err)
		return nil, err
	}

	return &Workspace{
		WorkspaceID:   workspaceID,
		WorkspaceName: name,
		MountPath:    mountPoint,
	}, nil
}

// GetWorkspace retrieves a workspace by ID
func GetWorkspace(db *sql.DB, workspaceID string) (*Workspace, error) {
	workspace := &Workspace{}

	err := db.QueryRow(`
		SELECT workspace_id, mount_path, workspace_name
		FROM workspaces WHERE workspace_id = ?
	`, workspaceID).Scan(
		&workspace.WorkspaceID,
		&workspace.MountPath,
		&workspace.WorkspaceName,
	)

	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil, nil
		}
		log.Printf("Failed to get workspace %s: %v", workspaceID, err)
		return nil, err
	}

	return workspace, nil
}

// GetAllWorkspaces retrieves all workspaces
func GetAllWorkspaces(db *sql.DB) ([]*Workspace, error) {
	rows, err := db.Query(`
		SELECT workspace_id, mount_path, workspace_name
		FROM workspaces ORDER BY workspace_name
	`)
	if err != nil {
		log.Printf("Failed to query workspaces: %v", err)
		return nil, err
	}
	defer rows.Close()

	var workspaces []*Workspace
	for rows.Next() {
		workspace := &Workspace{}
		err := rows.Scan(
			&workspace.WorkspaceID,
			&workspace.MountPath,
			&workspace.WorkspaceName,
		)
		if err != nil {
			log.Printf("Failed to scan workspace: %v", err)
			continue
		}
		workspaces = append(workspaces, workspace)
	}

	if err = rows.Err(); err != nil {
		log.Printf("Error iterating workspaces: %v", err)
		return nil, err
	}

	return workspaces, nil
}

// UpdateWorkspace updates a workspace's name and mount point
func UpdateWorkspace(db *sql.DB, workspaceID, name, mountPoint string) error {
	_, err := db.Exec(`
		UPDATE workspaces
		SET workspace_name = ?, mount_path = ?
		WHERE workspace_id = ?
	`, name, mountPoint, workspaceID)

	if err != nil {
		log.Printf("Failed to update workspace %s: %v", workspaceID, err)
		return err
	}

	return nil
}

// DeleteWorkspace removes a workspace
func DeleteWorkspace(db *sql.DB, workspaceID string) error {
	_, err := db.Exec(`DELETE FROM workspaces WHERE workspace_id = ?`, workspaceID)
	if err != nil {
		log.Printf("Failed to delete workspace %s: %v", workspaceID, err)
		return err
	}
	return nil
}
