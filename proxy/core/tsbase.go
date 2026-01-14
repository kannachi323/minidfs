package core

import (
	"context"
	"log"
	"sync"

	"tailscale.com/client/local"
	"tailscale.com/ipn"
	"tailscale.com/tsnet"
)

type TSStatus int

const (
	OFFLINE TSStatus = iota
	CONNECTED
	ERROR
	UNAUTHORIZED
)

type TSBase struct {
	TSServer	*tsnet.Server
	TSClient 	*local.Client
	TSState		*TSState
	mu			sync.Mutex
}

type TSState struct {
	Status TSStatus `json:"status"`
	AuthURL string `json:"auth_url"`
	ConnectedIP string `json:"connected_ip"`
}

type TSPeers struct {
	Peers []string `json:"peers"`
}

type TSPing struct {
	IP string `json:"ip"`
	Success bool   `json:"success"`
	Error   string `json:"error,omitempty"`
}


func CreateTSBase(dataDir string) *TSBase {
	server := &tsnet.Server{
		Hostname: "minidfs-node-v1",
		Dir:      dataDir,
		Logf: func(format string, args ...any) {},
	}
	localClient, err := server.LocalClient()
	if err != nil {
		log.Fatalf("Failed to create local client: %v", err)
	}

	return &TSBase{
        TSServer: server,
        TSClient: localClient,
        TSState: &TSState{
			Status: OFFLINE,
			AuthURL: "",
			ConnectedIP: "",
		},
    }
}

func (ts *TSBase) StartTSConnection() {
	go func() {
		watcher, err := ts.TSClient.WatchIPNBus(context.Background(), 0)
		if err != nil {
			log.Printf("Watcher error: %v", err)
			return
		}
		defer watcher.Close()

		for {
			n, err := watcher.Next()
			if err != nil {
				log.Printf("Watcher loop error: %v", err)
				return
			}

			ts.mu.Lock()
			if n.BrowseToURL != nil {
				ts.TSState.AuthURL = *n.BrowseToURL
				ts.TSState.Status = UNAUTHORIZED
			}

			if n.State != nil {
				switch *n.State {
				case ipn.Running:
					ts.TSState.Status = CONNECTED
                    status, _ := ts.TSClient.Status(context.Background())
                    if len(status.TailscaleIPs) > 0 {
                        ts.TSState.ConnectedIP = status.TailscaleIPs[0].String()
                    }
                    ts.TSState.AuthURL = ""
				case ipn.Stopped:
					ts.TSState.Status = OFFLINE
				case ipn.NeedsLogin:
					ts.TSState.Status = UNAUTHORIZED
				default:
					ts.TSState.Status = ERROR
				}
			}
			ts.mu.Unlock()
		}
	}()
}

func (ts* TSBase) GetStatus() (TSState) {
	ts.mu.Lock()
	defer ts.mu.Unlock()

	return *ts.TSState
}

func (ts* TSBase) GetPeers() (*TSPeers, error) {
	status, err := ts.TSClient.Status(context.Background())
	if err != nil {
		return nil, err
	}
	var peers []string
	for _, perr := range status.Peer {
		peers = append(peers, perr.HostName)
	}

	tsPeers := &TSPeers{
		Peers: peers,
	}
	return tsPeers, nil
}

func (ts *TSBase) PingPeer(hostname string) *TSPing {
	status, err := ts.TSClient.Status(context.Background())
	if err != nil {
		return &TSPing{
			IP: hostname,
			Success: false,
			Error: err.Error(),
		}
	}
	var targetIP string
	for _, peer := range status.Peer {
		if peer.HostName == hostname {
			for _, ip := range peer.TailscaleIPs {
				if ip.Is4() {
					targetIP = ip.String()
					break
				}
			}
		}
	}

	if targetIP == "" {
		return &TSPing{
			IP: hostname,
			Success: false,
			Error: "Peer not found",
		}
	}

	return &TSPing{
		IP: targetIP,
		Success: true,
	}
}
