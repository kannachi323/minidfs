package main

import (
	"net/http"

	"github.com/joho/godotenv"
)

func main() {
	godotenv.Load()
  	proxy := CreateProxy()
	proxy.MountHandlers()
	proxy.TSBase.StartTSConnection()
	
	if err := http.ListenAndServe(":3000", proxy.Router); err != nil {
    	panic(err)
	}
}