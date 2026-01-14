package api

import (
	"encoding/json"
	"net/http"

	"github.com/kannachi323/minidfs/proxy/core"
)

func GetTSStatus(ts *core.TSBase) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		data := ts.GetStatus()

        w.Header().Set("Content-Type", "application/json")
        json.NewEncoder(w).Encode(data)
	}
}

func GetPeers(ts *core.TSBase) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		peers, err := ts.GetPeers()
		if err != nil {
			http.Error(w, "Failed to get peers", http.StatusInternalServerError)
			return
		}
		
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(peers)
	}
}

func PingPeer(ts *core.TSBase) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		hostname := r.URL.Query().Get("hostname")
		if hostname == "" {
			http.Error(w, "Hostname parameter is required", http.StatusBadRequest)
			return
		}
		ping := ts.PingPeer(hostname)

		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(ping)
	}
}