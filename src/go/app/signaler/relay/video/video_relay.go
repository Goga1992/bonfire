package video_relay

import (
	"context"
	"fmt"
	"signaler/transcode"
	"sync"
	"time"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	bon_log "bonlib/log"
	bon_utils "bonlib/utils"
)

type VideoRelay struct {
	videoScalerConn   *grpc.ClientConn
	videoScalerClient transcode.VideoScalerClient

	mtx   sync.RWMutex
	slots map[string]*VideoSlot
}

type Subscriber struct {
	peerID string
	sink   chan *rtp.Packet
}

func NewVideoRelay() (*VideoRelay, error) {
	videoScalerConn, err := newVideoScalerConn()
	if err != nil {
		return nil, fmt.Errorf("could not connect to VideoScaler: %s", err)
	}

	return &VideoRelay{
		videoScalerConn:   videoScalerConn,
		videoScalerClient: transcode.NewVideoScalerClient(videoScalerConn),
		slots:             map[string]*VideoSlot{},
	}, nil
}

func newVideoScalerConn() (*grpc.ClientConn, error) {
	opts := []grpc.DialOption{
		grpc.WithTransportCredentials(insecure.NewCredentials()),
		grpc.WithBlock(),
	}

	videoScalerAddr, err := bon_utils.GetEnv("VIDEOSCALER_ADDR")
	if err != nil {
		return nil, err
	}

	ctx, cancel := context.WithTimeout(context.Background(), 3*time.Second)
	defer cancel()

	return grpc.DialContext(ctx, videoScalerAddr, opts...)
}

func (r *VideoRelay) AddSlot(track *webrtc.TrackRemote, peerID string) (*VideoSlot, error) {
	slot, err := NewVideoSlot(track, r.videoScalerClient)
	if err != nil {
		return nil, err
	}

	r.mtx.Lock()
	r.slots[peerID] = slot
	r.mtx.Unlock()

	slot.Start()

	return slot, err
}

func (r *VideoRelay) RemoveSlot(peerID string) {
	slot, ok := r.slots[peerID]
	if !ok {
		return
	}

	slot.Stop()

	delete(r.slots, peerID)
}

func (r *VideoRelay) Subscribe(peerID string, resolution string, subscriberPeerID string) error {
	slot, ok := r.slots[peerID]
	if !ok {
		return fmt.Errorf("could not find slot")
	}

	slot.ResolutionSubscribe(resolution, peerID)
	return nil
}

func (r *VideoRelay) Unsubscribe(peerID string, resolution string, subscriberPeerID string) error {
	slot, ok := r.slots[peerID]
	if !ok {
		return fmt.Errorf("could not find slot")
	}

	slot.ResolutionUnsubscribe(resolution, peerID)
	return nil
}

func (r *VideoRelay) GetPacketsChan(peerID string, resolution string, subscriberPeerID string) (chan *rtp.Packet, bool) {
	slot, ok := r.slots[peerID]
	if !ok {
		return nil, false
	}

	return slot.GetPacketsChan(resolution, peerID)
}

func (r *VideoRelay) SetIncomingResolution(peerID string, resolution string) {
	slot, ok := r.slots[peerID]
	if !ok {
		return
	}

	slot.SetIncomingResolution(resolution)
}

func (r *VideoRelay) ForceKeyFrame(peerID string, resolution string) {
	slot, ok := r.slots[peerID]
	if !ok {
		return
	}

	slot.ForceKeyFrame(resolution)
}

func (r *VideoRelay) Close() {
	for _, slot := range r.slots {
		slot.Stop()
	}

	err := r.videoScalerConn.Close()
	if err != nil {
		bon_log.Error.Printf("Could not close VideoScaler connection: %s", err)
	}
}
