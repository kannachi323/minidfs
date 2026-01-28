package api

import (
	"encoding/json"
	"net/http"

	"github.com/kannachi323/misty/proxy/core/tsbase"
	"github.com/kannachi323/misty/proxy/db"
)

func GetDevices(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		workspaceID := r.URL.Query().Get("workspace_id")

		var deviceList []*db.Device
		var err error

		if workspaceID != "" {
			deviceList, err = db.GetDevicesByWorkspace(database.Conn, workspaceID)
		} else {
			deviceList, err = db.GetAllDevices(database.Conn)
		}

		if err != nil {
			http.Error(w, "Failed to get db", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(deviceList)
	}
}

func RegisterDevice(ts *tsbase.TSBase, database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		// Get the server peer (current device)
		serverPeer := ts.GetServerPeer()
		if serverPeer == nil {
			http.Error(w, "Server peer not available", http.StatusBadRequest)
			return
		}

		// Parse request body for additional device information
		var deviceInfo struct {
			DeviceName string `json:"device_name"`
			MountPath  string `json:"mount_path"`
		}
		
		if r.Body != nil {
			decoder := json.NewDecoder(r.Body)
			if err := decoder.Decode(&deviceInfo); err != nil {
				// If JSON parsing fails, continue with empty values
				// (device_name and mount_path are optional)
			}
		}

		// Update/register the device in the database with additional info
		err := db.UpdateDevice(database.Conn, serverPeer, deviceInfo.DeviceName, deviceInfo.MountPath)
		if err != nil {
			http.Error(w, "Failed to register device", http.StatusInternalServerError)
			return
		}

		// Also sync all peers to the database
		allPeers, err := ts.GetPeers()
		if err == nil {
			db.SyncDevicesFromPeers(database.Conn, allPeers)
		}

		w.Header().Set("Content-Type", "application/json")
		response := map[string]interface{}{
			"status":  "success",
			"message": "Device registered successfully",
		}
		
		if deviceInfo.DeviceName != "" {
			response["device_name"] = deviceInfo.DeviceName
		}
		if deviceInfo.MountPath != "" {
			response["mount_path"] = deviceInfo.MountPath
		}
		
		json.NewEncoder(w).Encode(response)
	}
}

func UpdateDevice(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("id")
		if id == "" {
			http.Error(w, "Device ID is required", http.StatusBadRequest)
			return
		}

		var deviceInfo struct {
			DeviceName string `json:"device_name"`
			MountPath  string `json:"mount_path"`
		}

		if r.Body != nil {
			decoder := json.NewDecoder(r.Body)
			if err := decoder.Decode(&deviceInfo); err != nil {
				http.Error(w, "Invalid JSON in request body", http.StatusBadRequest)
				return
			}
		}

		err := db.UpdateDeviceInfo(database.Conn, id, deviceInfo.DeviceName, deviceInfo.MountPath)
		if err != nil {
			http.Error(w, "Failed to update device", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{
			"status":  "success",
			"message": "Device updated successfully",
		})
	}
}

func DeleteDevice(database *db.Database) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		id := r.URL.Query().Get("id")
		if id == "" {
			http.Error(w, "Device ID is required", http.StatusBadRequest)
			return
		}

		err := db.DeleteDevice(database.Conn, id)
		if err != nil {
			http.Error(w, "Failed to delete device", http.StatusInternalServerError)
			return
		}

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(map[string]string{
			"status":  "success",
			"message": "Device deleted successfully",
		})
	}
}
