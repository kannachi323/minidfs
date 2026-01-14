// Just to make sure i can reach my backend at any time XD

package api

import (
	"net/http"
)

func HelloWorld() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Write([]byte("Hello World"))
		w.WriteHeader(http.StatusOK)
	}
}