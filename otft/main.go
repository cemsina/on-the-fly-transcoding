package main

import (
	"log"
	"net/http"
	"os"
)

func main() {
	port := os.Getenv("PORT")
	if port == "" {
		port = "8080"
	}
	InitProfiles()
	PreparePatterns()
	http.HandleFunc("/", RouteHandler)
	log.Fatal(http.ListenAndServe(":"+port, nil))
}
