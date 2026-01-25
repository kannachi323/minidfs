package devices

import (
	"database/sql"
	"errors"
	"log"
	"time"

	"github.com/google/uuid"
	"github.com/kannachi323/misty/proxy/core/tsbase"
)

type Device struct {
	ID           string    `json:"id"`
	PeerHostName string    `json:"peer_hostname"`
	PeerType     string    `json:"peer_type"`
	PeerAddress  string    `json:"peer_address"`
	DeviceName   string    `json:"device_name,omitempty"`
	MountPath    string    `json:"mount_path,omitempty"`
	WorkspaceID  string    `json:"workspace_id,omitempty"`
	LastSeen     time.Time `json:"last_seen"`
	CreatedAt    time.Time `json:"created_at"`
	UpdatedAt    time.Time `json:"updated_at"`
}

// UpdateDevice inserts or updates a device. deviceName and mountPath are optional.
func UpdateDevice(db *sql.DB, peer *tsbase.TSPeer, deviceName, mountPath string) error {
	now := time.Now()

	deviceID, err := uuid.NewUUID()
	if err != nil {
		log.Println("failed to create device id")
		return err
	}
	
	_, err = db.Exec(`
		INSERT INTO devices (id, peer_hostname, peer_type, peer_address, device_name, mount_path, last_seen, created_at, updated_at)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)
		ON CONFLICT(peer_hostname) DO UPDATE SET
			peer_type = excluded.peer_type,
			peer_address = excluded.peer_address,
			device_name = COALESCE(excluded.device_name, devices.device_name),
			mount_path = COALESCE(excluded.mount_path, devices.mount_path),
			last_seen = excluded.last_seen,
			updated_at = excluded.updated_at
	`, deviceID, peer.PeerHostName, string(peer.PeerType), peer.PeerAddress, deviceName, mountPath, now, now, now)
	
	if err != nil {
		log.Printf("Failed to insert/update device %s: %v", peer.PeerHostName, err)
		return err
	}
	
	return nil
}

// GetDevice retrieves a device by hostname
func GetDevice(db *sql.DB, hostname string) (*Device, error) {
	device := &Device{}

	var workspaceID sql.NullString
	err := db.QueryRow(`
		SELECT id, peer_hostname, peer_type, peer_address, device_name, mount_path, workspace_id, last_seen, created_at, updated_at
		FROM devices WHERE peer_hostname = ?
	`, hostname).Scan(
		&device.ID,
		&device.PeerHostName,
		&device.PeerType,
		&device.PeerAddress,
		&device.DeviceName,
		&device.MountPath,
		&workspaceID,
		&device.LastSeen,
		&device.CreatedAt,
		&device.UpdatedAt,
	)

	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil, nil
		}
		log.Printf("Failed to get device %s: %v", hostname, err)
		return nil, err
	}

	device.WorkspaceID = workspaceID.String

	return device, nil
}

// GetDeviceByID retrieves a device by ID
func GetDeviceByID(db *sql.DB, id string) (*Device, error) {
	device := &Device{}

	var workspaceID sql.NullString
	err := db.QueryRow(`
		SELECT id, peer_hostname, peer_type, peer_address, device_name, mount_path, workspace_id, last_seen, created_at, updated_at
		FROM devices WHERE id = ?
	`, id).Scan(
		&device.ID,
		&device.PeerHostName,
		&device.PeerType,
		&device.PeerAddress,
		&device.DeviceName,
		&device.MountPath,
		&workspaceID,
		&device.LastSeen,
		&device.CreatedAt,
		&device.UpdatedAt,
	)

	if err != nil {
		if errors.Is(err, sql.ErrNoRows) {
			return nil, nil
		}
		log.Printf("Failed to get device %s: %v", id, err)
		return nil, err
	}

	device.WorkspaceID = workspaceID.String

	return device, nil
}

// DeleteDevice removes a device from the database by ID
func DeleteDevice(db *sql.DB, id string) error {
	_, err := db.Exec(`DELETE FROM devices WHERE id = ?`, id)
	if err != nil {
		log.Printf("Failed to delete device %s: %v", id, err)
		return err
	}
	return nil
}

// UpdateDeviceInfo updates device_name and mount_path for a device
func UpdateDeviceInfo(db *sql.DB, id string, deviceName, mountPath string) error {
	now := time.Now()
	
	_, err := db.Exec(`
		UPDATE devices 
		SET device_name = ?, mount_path = ?, updated_at = ?
		WHERE id = ?
	`, deviceName, mountPath, now, id)
	
	if err != nil {
		log.Printf("Failed to update device info %s: %v", id, err)
		return err
	}
	
	return nil
}

// GetAllDevices retrieves all devices from the database
func GetAllDevices(db *sql.DB) ([]*Device, error) {
	rows, err := db.Query(`
		SELECT id, peer_hostname, peer_type, peer_address, device_name, mount_path, workspace_id, last_seen, created_at, updated_at
		FROM devices ORDER BY peer_hostname
	`)
	if err != nil {
		log.Printf("Failed to query devices: %v", err)
		return nil, err
	}
	defer rows.Close()

	var devices []*Device
	for rows.Next() {
		device := &Device{}
		var workspaceID sql.NullString
		err := rows.Scan(
			&device.ID,
			&device.PeerHostName,
			&device.PeerType,
			&device.PeerAddress,
			&device.DeviceName,
			&device.MountPath,
			&workspaceID,
			&device.LastSeen,
			&device.CreatedAt,
			&device.UpdatedAt,
		)
		if err != nil {
			log.Printf("Failed to scan device: %v", err)
			continue
		}
		device.WorkspaceID = workspaceID.String
		devices = append(devices, device)
	}

	if err = rows.Err(); err != nil {
		log.Printf("Error iterating devices: %v", err)
		return nil, err
	}

	return devices, nil
}

// GetDevicesByWorkspace retrieves all devices belonging to a workspace
func GetDevicesByWorkspace(db *sql.DB, workspaceID string) ([]*Device, error) {
	rows, err := db.Query(`
		SELECT id, peer_hostname, peer_type, peer_address, device_name, mount_path, workspace_id, last_seen, created_at, updated_at
		FROM devices WHERE workspace_id = ? ORDER BY peer_hostname
	`, workspaceID)
	if err != nil {
		log.Printf("Failed to query devices by workspace: %v", err)
		return nil, err
	}
	defer rows.Close()

	var devices []*Device
	for rows.Next() {
		device := &Device{}
		var wsID sql.NullString
		err := rows.Scan(
			&device.ID,
			&device.PeerHostName,
			&device.PeerType,
			&device.PeerAddress,
			&device.DeviceName,
			&device.MountPath,
			&wsID,
			&device.LastSeen,
			&device.CreatedAt,
			&device.UpdatedAt,
		)
		if err != nil {
			log.Printf("Failed to scan device: %v", err)
			continue
		}
		device.WorkspaceID = wsID.String
		devices = append(devices, device)
	}

	if err = rows.Err(); err != nil {
		log.Printf("Error iterating devices: %v", err)
		return nil, err
	}

	return devices, nil
}

func SyncDevicesFromPeers(db *sql.DB, peers []*tsbase.TSPeer) error {
	for _, peer := range peers {
		if err := UpdateDevice(db, peer, "", ""); err != nil {
			log.Printf("Failed to sync device %s: %v", peer.PeerHostName, err)
			// Continue with other devices even if one fails
		}
	}
	return nil
}
