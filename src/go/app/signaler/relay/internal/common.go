package internal

import (
	"net"
	"sync"

	bon_log "bonlib/log"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
)

type InputSender struct {
	Track   *webrtc.TrackRemote
	UdpConn *net.UDPConn

	LastTimestamp uint32
}

type OutputReceiver struct {
	UdpConn     *net.UDPConn
	WorkerDone  chan any
	PacketsChan chan *rtp.Packet

	Mtx           sync.Mutex
	LastTimestamp uint32
}

func NewOutputReceiver() (*OutputReceiver, error) {
	udpConn, err := net.ListenUDP("udp", &net.UDPAddr{IP: net.ParseIP("127.0.0.1"), Port: 0})
	if err != nil {
		return nil, err
	}

	bon_log.Info.Printf("Listening for output on %v", udpConn.LocalAddr())
	return &OutputReceiver{udpConn, make(chan any), make(chan *rtp.Packet, 200), sync.Mutex{}, 0}, nil
}

func NewInputSender(track *webrtc.TrackRemote, audioMixerAddr string) (*InputSender, error) {
	raddr, err := net.ResolveUDPAddr("udp", audioMixerAddr)
	if err != nil {
		return nil, err
	}

	udpConn, err := net.DialUDP("udp", nil, raddr)
	if err != nil {
		return nil, err
	}

	return &InputSender{Track: track, UdpConn: udpConn}, nil
}

func SetDiffTimestamp(packet *rtp.Packet, lastTimestamp uint32) uint32 {
	oldTimestamp := packet.Timestamp
	if lastTimestamp == 0 {
		packet.Timestamp = 0
	} else {
		packet.Timestamp -= lastTimestamp
	}
	return oldTimestamp
}
