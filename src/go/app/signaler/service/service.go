package service

import (
	bon_log "bonlib/log"
	"html/template"
	"io/ioutil"
	"net/http"

	"github.com/gorilla/websocket"
)

type SignalerService struct {
	upgrader      websocket.Upgrader
	indexTemplate *template.Template
	room          *Room
}

func NewSignalerService() (*SignalerService, error) {
	indexHTML, err := ioutil.ReadFile("client_browser/index.html")
	if err != nil {
		return nil, err
	}

	room, err := NewRoom()
	if err != nil {
		return nil, err
	}

	service := &SignalerService{
		upgrader: websocket.Upgrader{
			CheckOrigin: func(r *http.Request) bool { return true },
		},
		indexTemplate: template.Must(template.New("").Parse(string(indexHTML))),
		room:          room,
	}

	// go func() {
	// 	time.Sleep(3 * time.Second)
	// 	service.room.DispatchKeyFrame()
	// }()

	service.registerHandlers()

	return service, nil
}

func (s *SignalerService) registerHandlers() {
	// websocket handler
	http.HandleFunc("/websocket", s.websocketHandler)

	// index.html handler
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		if err := s.indexTemplate.Execute(w, "wss://"+r.Host+"/websocket"); err != nil {
			bon_log.Error.Fatal(err)
		}
	})

}

func (s *SignalerService) Start() {
	// request a keyframe every 3 seconds

	bon_log.Info.Println("Listening on port :443")
	err := http.ListenAndServeTLS(":443", "tools/server.crt", "tools/server.key", nil)
	if err != http.ErrServerClosed {
		bon_log.Error.Fatal(err)
	}
}

// Handle incoming websockets
func (s *SignalerService) websocketHandler(w http.ResponseWriter, r *http.Request) {
	// Upgrade HTTP request to Websocket
	unsafeConn, err := s.upgrader.Upgrade(w, r, nil)
	if err != nil {
		return
	}

	s.room.AddPeer(unsafeConn)
}
