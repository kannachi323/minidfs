package main

import (
	"net/http"
	"os"
	"path/filepath"

	"github.com/go-chi/chi/v5"
	"github.com/go-chi/cors"
	"github.com/kannachi323/misty/proxy/api"
	"github.com/kannachi323/misty/proxy/api/ms"
	"github.com/kannachi323/misty/proxy/core/tsbase"
	"github.com/kannachi323/misty/proxy/db"
)

type Proxy struct {
	Router       *chi.Mux
	APIRouter	 *chi.Mux
	TSBase		 *tsbase.TSBase
	Database     *db.Database
}

func CreateProxy() (*Proxy, error) {
	home, _ := os.UserHomeDir()
	dataDir := filepath.Join(home, "misty", "minidfs", "tailscale")
	os.MkdirAll(dataDir, 0700)

	proxy := &Proxy{
		Router: chi.NewRouter(),
	}
	proxy.Router.Route("/api", func(r chi.Router) {
		proxy.APIRouter = r.(*chi.Mux)
	})
	
	// Serve static files
	workDir, _ := os.Getwd()
	staticDir := filepath.Join(workDir, "static")
	proxy.Router.Handle("/static/*", http.StripPrefix("/static/", http.FileServer(http.Dir(staticDir))))
	base, err := tsbase.CreateTSBase(dataDir)
	if err != nil {
		return nil, err
	}
	proxy.TSBase = base
	proxy.Database = &db.Database{}

	return proxy, nil
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
	
	//////--------------------------
	// DO NOT REMOVE THIS
	proxy.APIRouter.Get("/hello", api.HelloWorld())
	//////--------------------------

	proxy.APIRouter.Get("/ts-status", api.GetTSStatus(proxy.TSBase));
	proxy.APIRouter.Get("/ts-peers", api.GetPeers(proxy.TSBase));
	proxy.APIRouter.Get("/ts-ping", api.PingServer(proxy.TSBase))

	proxy.APIRouter.Post("/register", api.RegisterUser(proxy.Database))
	proxy.APIRouter.Post("/login", api.LoginUser(proxy.Database))

	proxy.APIRouter.Post("/file", api.CreateFile(proxy.Database))
	proxy.APIRouter.Get("/file", api.GetFile(proxy.Database))

	proxy.APIRouter.Get("/devices", api.GetDevices(proxy.Database))
	proxy.APIRouter.Post("/devices", api.RegisterDevice(proxy.TSBase, proxy.Database))
	proxy.APIRouter.Put("/devices", api.UpdateDevice(proxy.Database))
	proxy.APIRouter.Delete("/devices", api.DeleteDevice(proxy.Database))

	proxy.APIRouter.Get("/workspaces", api.GetWorkspaces(proxy.Database))
	proxy.APIRouter.Get("/workspace", api.GetWorkspace(proxy.Database))
	proxy.APIRouter.Post("/workspaces", api.CreateWorkspace(proxy.Database))
	proxy.APIRouter.Put("/workspaces", api.UpdateWorkspace(proxy.Database))
	proxy.APIRouter.Delete("/workspaces", api.DeleteWorkspace(proxy.Database))

	// Microsoft OAuth endpoints
	proxy.APIRouter.Get("/ms/auth", ms.GetOAuthLogin())
	proxy.APIRouter.Get("/ms/callback", ms.OAuthCallback(proxy.Database))
	proxy.APIRouter.Get("/ms/callback/token", ms.UpdateMSToken(proxy.Database))
	proxy.APIRouter.Get("/ms/token", ms.GetMSTokens(proxy.Database))
	proxy.APIRouter.Post("/ms/token/refresh", ms.RefreshMSToken(proxy.Database))
	proxy.APIRouter.Delete("/ms/token", ms.DeleteMSToken(proxy.Database))

	
}