package peer

import (
	bon_log "bonlib/log"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"strconv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/gorilla/websocket"
	"github.com/pion/rtcp"
	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
)

var OUT_VIDEOS_NUM = 10

type Peer struct {
	id string

	conn     *webrtc.PeerConnection
	wsWriter *threadSafeWriter

	outcomingAudioTracks []*outcomingTrack
	outcomingVideoTracks []*outcomingTrack

	gotTrackChan chan *webrtc.TrackRemote

	participantsChangeChan *participantsChangeChannel
	resolutionChangedChan  *resolutionChangedChannel
	resolutionWantedChan   *resolutionWantedChannel

	cleanupRequestedChan chan bool
	stopWorkers          atomic.Value

	websocketDone chan any
	allDone       chan any
}

type outcomingTrack struct {
	srcMtx sync.Mutex
	src    chan *rtp.Packet

	sink *webrtc.RTPSender

	done chan any
}

type websocketMessage struct {
	Event string `json:"event"`
	Data  string `json:"data"`
}

func NewPeer(peerID string, websocketConn *websocket.Conn) (*Peer, error) {
	p := &Peer{}

	// Upgrade HTTP request to Websocket
	wsWriter := &threadSafeWriter{websocketConn, sync.Mutex{}}

	// Create new PeerConnection
	peerConnection, err := webrtc.NewPeerConnection(
		webrtc.Configuration{
			ICEServers: []webrtc.ICEServer{
				{
					URLs: []string{"stun:stun.l.google.com:19302"},
				},
			},
		},
	)
	if err != nil {
		return nil, fmt.Errorf("could not create peer connection: %w", err)
	}

	// Accept one audio and one video track incoming
	for _, typ := range []webrtc.RTPCodecType{webrtc.RTPCodecTypeVideo, webrtc.RTPCodecTypeAudio} {
		trans, err := peerConnection.AddTransceiverFromKind(
			typ,
			webrtc.RTPTransceiverInit{Direction: webrtc.RTPTransceiverDirectionRecvonly},
		)
		if err != nil {
			return nil, fmt.Errorf("could not create transceiver: %w", err)
		}

		if typ == webrtc.RTPCodecTypeVideo {
			trans.SetCodecPreferences(
				[]webrtc.RTPCodecParameters{
					{
						RTPCodecCapability: webrtc.RTPCodecCapability{MimeType: webrtc.MimeTypeH264},
						PayloadType:        96,
					},
				},
			)
		}
	}

	p.id = peerID
	p.conn = peerConnection
	p.wsWriter = wsWriter
	p.conn.OnICECandidate(p.onICECandidateCallback)
	p.conn.OnConnectionStateChange(p.onConnectionStateChangeCallback)
	p.conn.OnTrack(p.onTrackCallback)
	p.conn.OnNegotiationNeeded(func() {
		bon_log.Error.Println("Negotiation needed")
	})

	resolutionChangedChan, err := p.newResolutionChangedChannel()
	if err != nil {
		return nil, fmt.Errorf("could not create ResolutionChangedChannel: %w", err)
	}

	resolutionWantedChan, err := p.newResolutionWantedChannel()
	if err != nil {
		return nil, fmt.Errorf("could not create ResolutionWantedChannel: %w", err)
	}

	participantsChangeChan, err := p.newParticipantsChangeChannel()
	if err != nil {
		return nil, fmt.Errorf("could not create ParticipantsChangeChannel: %w", err)
	}

	p.outcomingAudioTracks = []*outcomingTrack{}
	p.outcomingVideoTracks = []*outcomingTrack{}
	p.gotTrackChan = make(chan *webrtc.TrackRemote)
	p.participantsChangeChan = participantsChangeChan
	p.resolutionChangedChan = resolutionChangedChan
	p.resolutionWantedChan = resolutionWantedChan
	p.stopWorkers = atomic.Value{}
	p.cleanupRequestedChan = make(chan bool)
	p.websocketDone = make(chan any)
	p.allDone = make(chan any)

	p.stopWorkers.Store(false)

	p.prepareOutcomingTracks()

	go p.cleanupWorker()
	go p.webSocketWorker()
	go func() {
		for {
			p.RequestKeyFrame()
			time.Sleep(3 * time.Second)
		}
	}()

	return p, nil
}

func (p *Peer) prepareOutcomingTracks() error {
	for i := 0; i < OUT_VIDEOS_NUM; i++ {
		dummyVideoTrack, err := webrtc.NewTrackLocalStaticRTP(
			webrtc.RTPCodecCapability{
				MimeType:  webrtc.MimeTypeH264,
				ClockRate: 90000,
			},
			strconv.Itoa(i),
			strconv.Itoa(i),
		)
		if err != nil {
			return fmt.Errorf("could not prepare local video track: %w", err)
		}

		senderVideo, err := p.conn.AddTrack(dummyVideoTrack)
		if err != nil {
			return fmt.Errorf("could not add dummy video track: %w", err)
		}

		p.outcomingVideoTracks = append(p.outcomingVideoTracks, &outcomingTrack{sync.Mutex{}, make(chan *rtp.Packet, 200), senderVideo, make(chan any)})
	}

	dummyAudioTrack, err := webrtc.NewTrackLocalStaticRTP(
		webrtc.RTPCodecCapability{
			MimeType:  webrtc.MimeTypeOpus,
			Channels:  1,
			ClockRate: 90000,
		},
		strconv.Itoa(OUT_VIDEOS_NUM),
		strconv.Itoa(OUT_VIDEOS_NUM),
	)
	if err != nil {
		return fmt.Errorf("could not prepare local audio track: %w", err)
	}

	senderAudio, err := p.conn.AddTrack(dummyAudioTrack)
	if err != nil {
		return fmt.Errorf("could not add dummy audio track: %w", err)
	}
	p.outcomingAudioTracks = append(p.outcomingAudioTracks, &outcomingTrack{sync.Mutex{}, make(chan *rtp.Packet, 200), senderAudio, make(chan any)})

	for _, out := range p.outcomingVideoTracks {
		go p.outcomingTrackWorker(out)
	}

	for _, out := range p.outcomingAudioTracks {
		go p.outcomingTrackWorker(out)
	}

	offer, err := p.conn.CreateOffer(nil)
	if err != nil {
		return fmt.Errorf("could not create offer: %w", err)
	}

	err = p.conn.SetLocalDescription(offer)
	if err != nil {
		return fmt.Errorf("could not set local description: %w", err)
	}

	offerString, err := json.Marshal(offer)
	if err != nil {
		return fmt.Errorf("could not marshal offer: %w", err)
	}

	err = p.wsWriter.WriteJSON(&websocketMessage{
		Event: "offer",
		Data:  string(offerString),
	})
	if err != nil {
		return fmt.Errorf("could not send offer: %w", err)
	}

	return nil
}

func (p *Peer) OutcomingVideoTrackChan(trackIdx int) chan *rtp.Packet {
	return p.outcomingVideoTracks[trackIdx].src
}

func (p *Peer) OutcomingAudioTrackChan(trackIdx int) chan *rtp.Packet {
	return p.outcomingAudioTracks[trackIdx].src
}

func (p *Peer) outcomingTrackWorker(out *outcomingTrack) {
	var currTimestamp uint32
	for i := uint16(0); ; i++ {
		var packetRef *rtp.Packet
		var more bool

		for {
			out.srcMtx.Lock()
			select {
			case packetRef, more = <-out.src:
			case <-time.After(1 * time.Second):
				out.srcMtx.Unlock()
				continue
			case <-out.done:
				out.srcMtx.Unlock()
				return
			}

			out.srcMtx.Unlock()
			break
		}

		if !more {
			continue
		}

		packet := *packetRef

		// // Timestamp on the packet is really a diff, so add it to current
		currTimestamp += packet.Timestamp
		packet.Timestamp = currTimestamp
		// if out.sink.Track().Kind() == webrtc.RTPCodecTypeVideo {
		// 	bon_log.Debug.Printf("PID: %s, TS: %d", p.ID(), packet.Timestamp)
		// }

		// Keep an increasing sequence number
		packet.SequenceNumber = i
		// Write out the packet, ignoring closed pipe if nobody is listening
		err := out.sink.Track().(*webrtc.TrackLocalStaticRTP).WriteRTP(&packet)
		if err != nil {
			if errors.Is(err, io.ErrClosedPipe) {
				// The peerConnection has been closed.
				return
			}

			bon_log.Error.Println(err)
		}
	}
}

func (p *Peer) ReplaceVideoTrack(newSrc chan *rtp.Packet, oldTrackIdx int) {
	out := p.outcomingVideoTracks[oldTrackIdx]
	if out.src == newSrc {
		return
	}

	out.srcMtx.Lock()
	out.src = newSrc
	out.srcMtx.Unlock()
}

func (p *Peer) ReplaceAudioTrack(newSrc chan *rtp.Packet, oldTrackIdx int) {
	out := p.outcomingAudioTracks[oldTrackIdx]
	if out.src == newSrc {
		return
	}

	out.srcMtx.Lock()
	out.src = newSrc
	out.srcMtx.Unlock()
}

func (p *Peer) ID() string {
	return p.id
}

func (p *Peer) onICECandidateCallback(candidate *webrtc.ICECandidate) {
	if candidate == nil {
		return
	}

	candidateString, err := json.Marshal(candidate.ToJSON())
	if err != nil {
		bon_log.Error.Println(err)
		return
	}

	err = p.wsWriter.WriteJSON(
		&websocketMessage{
			Event: "candidate",
			Data:  string(candidateString),
		},
	)
	if err != nil {
		bon_log.Error.Println(err)
	}
}

func (p *Peer) onConnectionStateChangeCallback(state webrtc.PeerConnectionState) {
	switch state {
	case webrtc.PeerConnectionStateFailed:
		p.requestCleanup()
	case webrtc.PeerConnectionStateClosed:
		p.requestCleanup()
	}
}

func (p *Peer) onTrackCallback(t *webrtc.TrackRemote, _ *webrtc.RTPReceiver) {
	bon_log.Info.Printf("Got remote track: trackID=[%s], codec=[%s]", t.ID(), t.Codec().MimeType)
	p.gotTrackChan <- t
}

func (p *Peer) webSocketWorker() {
	defer func() {
		// close websocket connection
		err := p.wsWriter.Close()
		if err != nil {
			bon_log.Error.Printf("Could not close websocket: %s", err)
		}

		// signal that websocket is done
		close(p.websocketDone)

		p.requestCleanup()
	}()

	message := &websocketMessage{}

	for !p.stopWorkers.Load().(bool) {
		_, raw, err := p.wsWriter.ReadMessage()
		if err != nil {
			bon_log.Error.Println(err)
			return
		} else if err := json.Unmarshal(raw, &message); err != nil {
			bon_log.Error.Println(err)
			return
		}

		switch message.Event {
		case "candidate":
			candidate := webrtc.ICECandidateInit{}
			if err := json.Unmarshal([]byte(message.Data), &candidate); err != nil {
				bon_log.Error.Println(err)
				return
			}

			if err := p.conn.AddICECandidate(candidate); err != nil {
				bon_log.Error.Println(err)
				return
			}
		case "answer":
			// bon_log.Debug.Printf("Received answer: peerID=[%s]", peerID)
			answer := webrtc.SessionDescription{}
			if err := json.Unmarshal([]byte(message.Data), &answer); err != nil {
				bon_log.Error.Println(err)
				return
			}

			if err := p.conn.SetRemoteDescription(answer); err != nil {
				bon_log.Error.Println(err)
				return
			}
		}
	}
}

func (p *Peer) GotTrackChan() chan *webrtc.TrackRemote {
	return p.gotTrackChan
}

func (p *Peer) ParticipantsChangeChan() chan []string {
	return p.participantsChangeChan.Src()
}

func (p *Peer) ResolutionChangedChan() chan string {
	return p.resolutionChangedChan.Sink()
}

func (p *Peer) ResolutionWantedChan() chan map[string]string {
	return p.resolutionWantedChan.Sink()
}

func (p *Peer) RequestKeyFrame() {
	for _, receiver := range p.conn.GetReceivers() {
		if receiver.Track() == nil || receiver.Track().Kind() != webrtc.RTPCodecTypeVideo {
			continue
		}

		_ = p.conn.WriteRTCP(
			[]rtcp.Packet{
				&rtcp.PictureLossIndication{
					MediaSSRC: uint32(receiver.Track().SSRC()),
				},
			},
		)
	}
}

func (p *Peer) requestCleanup() {
	select {
	case p.cleanupRequestedChan <- true:
	default:
	}
}

func (p *Peer) cleanupWorker() {
	// wait until cleanup requested
	<-p.cleanupRequestedChan

	// stop websocket worker
	p.stopWorkers.Store(true)

	for _, out := range p.outcomingAudioTracks {
		close(out.done)
	}

	for _, out := range p.outcomingVideoTracks {
		close(out.done)
	}

	// depends on peer connection
	<-p.websocketDone

	p.participantsChangeChan.Close()
	p.resolutionWantedChan.Close()
	p.resolutionChangedChan.Close()

	// depends on gotTrackChan
	err := p.conn.Close()
	if err != nil {
		bon_log.Error.Printf("Could not close peer connection: %s", err)
	}

	// close(p.gotTrackChan)

	close(p.allDone)
}

func (p *Peer) DoneChan() chan any {
	return p.allDone
}

func (p *Peer) Join() {
	<-p.allDone
}

func (p *Peer) Close() {
	p.requestCleanup()
	p.Join()
}

// Helper to make Gorilla Websockets threadsafe
type threadSafeWriter struct {
	*websocket.Conn
	sync.Mutex
}

func (t *threadSafeWriter) WriteJSON(v interface{}) error {
	t.Lock()
	defer t.Unlock()

	return t.Conn.WriteJSON(v)
}
