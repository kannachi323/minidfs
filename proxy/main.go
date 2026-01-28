package main

import (
	"net/http"
)

func main() {
  	proxy, err := CreateProxy()
	if err != nil {
		panic(err)
	}

	if err := proxy.Database.StartDatabase(); err != nil {
		panic(err)
	}

	proxy.MountHandlers()
	proxy.TSBase.StartTSConnection()

	if err := http.ListenAndServe("127.0.0.1:3000", proxy.Router); err != nil {
    	panic(err)
	}

	proxy.Database.Stop()
	
}