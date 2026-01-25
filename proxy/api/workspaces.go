package api

import (
	"encoding/json"
	"net/http"

	"github.com/kannachi323/misty/proxy/core/workspaces"
	"github.com/kannachi323/misty/proxy/db"
)

func GetWorkspaces(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		workspaceList, err := workspaces.GetAllWorkspaces(database.Conn)
		if err != nil {
			http.Error(w, "Failed to get workspaces", http.StatusInternalServerError)
			return
		}

		// Ensure we return an empty array instead of null
		if workspaceList == nil {
			workspaceList = []*workspaces.Workspace{}
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(workspaceList)
	}
}

func GetWorkspace(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("id")
		if id == "" {
			http.Error(w, "Workspace ID is required", http.StatusBadRequest)
			return
		}

		workspace, err := workspaces.GetWorkspace(database.Conn, id)
		if err != nil {
			http.Error(w, "Failed to get workspace", http.StatusInternalServerError)
			return
		}

		if workspace == nil {
			http.Error(w, "Workspace not found", http.StatusNotFound)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(workspace)
	}
}

func CreateWorkspace(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		var req struct {
			WorkspaceName string `json:"workspace_name"`
			MountPath    string `json:"mount_point"`
		}

		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "Invalid JSON in request body", http.StatusBadRequest)
			return
		}

		if req.WorkspaceName == "" || req.MountPath == "" {
			http.Error(w, "workspace_name and mount_point are required", http.StatusBadRequest)
			return
		}

		workspace, err := workspaces.CreateWorkspace(database.Conn, req.WorkspaceName, req.MountPath)
		if err != nil {
			http.Error(w, "Failed to create workspace", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusCreated)
		json.NewEncoder(w).Encode(workspace)
	}
}

func UpdateWorkspace(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("id")
		if id == "" {
			http.Error(w, "Workspace ID is required", http.StatusBadRequest)
			return
		}

		var req struct {
			WorkspaceName string `json:"workspace_name"`
			MountPath    string `json:"mount_point"`
		}

		if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
			http.Error(w, "Invalid JSON in request body", http.StatusBadRequest)
			return
		}

		err := workspaces.UpdateWorkspace(database.Conn, id, req.WorkspaceName, req.MountPath)
		if err != nil {
			http.Error(w, "Failed to update workspace", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{
			"status":  "success",
			"message": "Workspace updated successfully",
		})
	}
}

func DeleteWorkspace(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("id")
		if id == "" {
			http.Error(w, "Workspace ID is required", http.StatusBadRequest)
			return
		}

		err := workspaces.DeleteWorkspace(database.Conn, id)
		if err != nil {
			http.Error(w, "Failed to delete workspace", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{
			"status":  "success",
			"message": "Workspace deleted successfully",
		})
	}
}
