package audio_relay

import (
	bon_utils "bonlib/utils"
	"context"
	"fmt"
	"signaler/transcode"
	"sync"
	"time"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type AudioRelay struct {
	audioScalerConn   *grpc.ClientConn
	audioScalerClient transcode.AudioMixerClient

	roomID string

	mtx   sync.RWMutex
	slots map[string]*AudioSlot
}

func NewAudioRelay(roomID string) (*AudioRelay, error) {
	audioScalerConn, err := newAudioMixerConn()
	if err != nil {
		return nil, fmt.Errorf("could not connect to AudioMixer: %s", err)
	}

	return &AudioRelay{
		audioScalerConn:   audioScalerConn,
		audioScalerClient: transcode.NewAudioMixerClient(audioScalerConn),

		roomID: roomID,

		slots: map[string]*AudioSlot{},
	}, nil
}

func newAudioMixerConn() (*grpc.ClientConn, error) {
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithBlock(),
	}

	audioMixerAddr, err := bon_utils.GetEnv("AUDIOMIXER_ADDR")
	if err != nil {
		return nil, err
	}

	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()

	return grpc.DialContext(ctx, audioMixerAddr, opts...)
}

func (r *AudioRelay) AddSlot(track *webrtc.TrackRemote, peerID string) (*AudioSlot, error) {
	slot, err := NewAudioSlot(track, r.roomID, r.audioScalerClient)
	if err != nil {
		return nil, err
	}

	r.mtx.Lock()
	r.slots[peerID] = slot
	r.mtx.Unlock()

	slot.Start()

	return slot, nil
}

func (r *AudioRelay) GetTrack(peerID string) (chan *rtp.Packet, bool) {
	r.mtx.RLock()
	defer r.mtx.RUnlock()

	slot, ok := r.slots[peerID]
	if !ok {
		return nil, ok
	}
	return slot.GetTrack(), ok
}

func (r *AudioRelay) RemoveSlot(peerID string) {
	r.mtx.Lock()
	defer r.mtx.Unlock()

	slot, ok := r.slots[peerID]
	if !ok {
		return
	}
	delete(r.slots, peerID)

	slot.Stop()

}

func (r *AudioRelay) Close() {
	for _, slot := range r.slots {
		slot.Stop()
	}
}
