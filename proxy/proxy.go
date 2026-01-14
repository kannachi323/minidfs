package main

import (
	"os"
	"path/filepath"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/cors"
	"github.com/kannachi323/minidfs/proxy/api"
	"github.com/kannachi323/minidfs/proxy/core"
)

type Proxy struct {
	Router       *chi.Mux
	APIRouter	 *chi.Mux
	TSBase		 *core.TSBase
}

func CreateProxy() *Proxy {
	home, _ := os.UserHomeDir()
	dataDir := filepath.Join(home, ".minidfs", "tailscale")
	os.MkdirAll(dataDir, 0700)

	s := &Proxy{
		Router: chi.NewRouter(),
		TSBase: core.CreateTSBase(dataDir),
	}
	s.Router.Route("/api", func(r chi.Router) {
		s.APIRouter = r.(*chi.Mux)
	})

	return s
}


func (proxy *Proxy) MountHandlers() {
	proxy.APIRouter.Use(cors.Handler(cors.Options{
		AllowedOrigins:   []string{"https://*", "http://*"},
		AllowedMethods:   []string{"GET", "POST", "PUT", "DELETE", "OPTIONS"},
		AllowedHeaders:   []string{"Accept", "Authorization", "Content-Type", "X-CSRF-Token"},
		ExposedHeaders:   []string{"Link"},
		AllowCredentials: true,
		MaxAge:           300, // Maximum value not ignored by any of major browsers
	}))
	
	//DO NOT REMOVE THIS
	proxy.APIRouter.Get("/hello", api.HelloWorld())

	proxy.APIRouter.Get("/auth", api.GetTSStatus(proxy.TSBase));
	proxy.APIRouter.Get("/peers", api.GetPeers(proxy.TSBase));
	proxy.APIRouter.Get("/ping", api.PingPeer(proxy.TSBase))
}