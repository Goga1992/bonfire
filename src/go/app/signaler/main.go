package main

import (
	bon_log "bonlib/log"
	"signaler/service"
)

func main() {
	service, err := service.NewSignalerService()
	if err != nil {
		bon_log.Error.Fatal(err)
	}
	service.Start()
}
