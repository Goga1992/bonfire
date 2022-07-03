package service

import (
	bon_log "bonlib/log"
	"sync"

	"signaler/peer"
	audio_relay "signaler/relay/audio"
	video_relay "signaler/relay/video"

	"github.com/google/uuid"
	"github.com/gorilla/websocket"
	"github.com/pion/webrtc/v3"
)

type Room struct {
	peersMtx sync.RWMutex
	peers    map[string]*peer.Peer
	peersWg  sync.WaitGroup

	videoRelay *video_relay.VideoRelay
	audioRelay *audio_relay.AudioRelay

	peerToSubscriptions map[string]map[string]string
	trackToPeer         map[string][]string
}

func NewRoom() (*Room, error) {
	videoRelay, err := video_relay.NewVideoRelay()
	if err != nil {
		return nil, err
	}

	audioRelay, err := audio_relay.NewAudioRelay("kek")
	if err != nil {
		return nil, err
	}

	return &Room{
		peersMtx:            sync.RWMutex{},
		peers:               map[string]*peer.Peer{},
		peersWg:             sync.WaitGroup{},
		videoRelay:          videoRelay,
		audioRelay:          audioRelay,
		peerToSubscriptions: map[string]map[string]string{},
		trackToPeer:         map[string][]string{},
	}, nil
}

func (r *Room) AddPeer(websocket *websocket.Conn) {
	peerUUID, err := uuid.NewUUID()
	if err != nil {
		bon_log.Error.Printf("Could not create UUID: %s", err)
		return
	}

	peerID := peerUUID.String()

	peer, err := peer.NewPeer(peerID, websocket)
	if err != nil {
		bon_log.Error.Printf("Could not create peer: %s", err)
		return
	}

	r.peersMtx.Lock()
	r.peers[peerID] = peer
	r.peerToSubscriptions[peerID] = map[string]string{}
	r.trackToPeer[peerID] = make([]string, 10)
	r.peersMtx.Unlock()

	go r.peerLifeWorker(peer)

	r.signalParticipantChange(peer, "enter")
}

func (r *Room) peerLifeWorker(peer *peer.Peer) {
	defer func() {
		r.peersMtx.Lock()
		peer.Close()
		r.removePeerTracks(peer)
		delete(r.peers, peer.ID())
		delete(r.peerToSubscriptions, peer.ID())
		delete(r.trackToPeer, peer.ID())
		r.peersMtx.Unlock()

		r.signalParticipantChange(peer, "exit")
		r.updateOutcomingTracks()

		r.peersWg.Done()
	}()
	r.peersWg.Add(1)

	for {
		select {
		case track, more := <-peer.GotTrackChan():
			if !more {
				return
			}
			r.onGotTrack(track, peer)
		case resolution, more := <-peer.ResolutionChangedChan():
			if !more {
				return
			}
			r.onResolutionChanged(resolution, peer)
		case wanted, more := <-peer.ResolutionWantedChan():
			if !more {
				return
			}
			r.onResolutionsWanted(wanted, peer)
		case <-peer.DoneChan():
			return
		}
	}
}

func (r *Room) DispatchKeyFrame() {
	r.peersMtx.Lock()
	defer r.peersMtx.Unlock()

	for _, peer := range r.peers {
		peer.RequestKeyFrame()
	}
}

func (r *Room) updateOutcomingTracks() {
	r.peersMtx.Lock()
	defer func() {
		r.peersMtx.Unlock()
		r.DispatchKeyFrame()
	}()

	for _, peer := range r.peers {
		newAudioTrack, ok := r.audioRelay.GetTrack(peer.ID())
		if ok {
			bon_log.Info.Printf("Replacing audio track: peerID=[%s]", peer.ID())
			peer.ReplaceAudioTrack(newAudioTrack, 0)
		}

		for otherPeerID, resolution := range r.peerToSubscriptions[peer.ID()] {
			idx := -1
			for i, ownedBy := range r.trackToPeer[peer.ID()] {
				if ownedBy == otherPeerID {
					idx = i
					break
				}
				if ownedBy == "" {
					idx = i
					r.trackToPeer[peer.ID()][i] = otherPeerID
					break
				}
				if _, ok := r.peerToSubscriptions[ownedBy]; !ok {
					idx = i
					r.trackToPeer[peer.ID()][i] = otherPeerID
					break
				}
			}
			if idx == -1 {
				continue
			}

			packetsChan, ok := r.videoRelay.GetPacketsChan(otherPeerID, resolution, peer.ID())
			if !ok {
				continue
			}

			peer.ReplaceVideoTrack(packetsChan, idx)
		}
	}
}

func (r *Room) signalParticipantChange(peer *peer.Peer, change string) {
	r.peersMtx.Lock()
	defer r.peersMtx.Unlock()

	for _, otherPeer := range r.peers {
		if otherPeer == peer {
			continue
		}

		select {
		case otherPeer.ParticipantsChangeChan() <- []string{peer.ID(), change}:
		case <-otherPeer.DoneChan():
		}

		if change == "enter" {
			select {
			case peer.ParticipantsChangeChan() <- []string{otherPeer.ID(), change}:
			case <-peer.DoneChan():
			}
		}
	}
}

func (r *Room) onGotTrack(track *webrtc.TrackRemote, peer *peer.Peer) {
	r.peersMtx.Lock()
	defer func() {
		r.peersMtx.Unlock()
		r.updateOutcomingTracks()
	}()

	if track.Kind() == webrtc.RTPCodecTypeVideo {
		bon_log.Info.Printf("Adding video track: trackID=[%s]", track.ID())
		r.videoRelay.AddSlot(track, peer.ID())
	} else {
		bon_log.Info.Printf("Adding audio track: trackID=[%s]", track.ID())
		r.audioRelay.AddSlot(track, peer.ID())
	}
}

func (r *Room) removePeerTracks(peer *peer.Peer) {
	for otherPeerID, resolution := range r.peerToSubscriptions[peer.ID()] {
		r.videoRelay.Subscribe(otherPeerID, resolution, peer.ID())
	}

	r.audioRelay.RemoveSlot(peer.ID())
	r.videoRelay.RemoveSlot(peer.ID())
}

func (r *Room) onResolutionChanged(resolution string, peer *peer.Peer) {
	r.peersMtx.Lock()
	defer r.peersMtx.Unlock()

	r.videoRelay.SetIncomingResolution(peer.ID(), resolution)
}

func (r *Room) onResolutionsWanted(wanted map[string]string, peer *peer.Peer) {
	r.peersMtx.Lock()
	defer func() {
		r.peersMtx.Unlock()
		r.updateOutcomingTracks()
	}()

	couldSubscribe := map[string]string{}

	for otherPeerID, resolution := range wanted {
		r.videoRelay.Subscribe(otherPeerID, resolution, peer.ID())
		couldSubscribe[otherPeerID] = resolution
	}

	for otherPeerID, resolution := range r.peerToSubscriptions[peer.ID()] {
		if wanted[otherPeerID] == resolution {
			continue
		}

		r.videoRelay.Unsubscribe(otherPeerID, resolution, peer.ID())
	}

	r.peerToSubscriptions[peer.ID()] = couldSubscribe
}

func (r *Room) Close() {
	r.peersMtx.Lock()
	defer r.peersMtx.Unlock()

	for _, peer := range r.peers {
		peer.Close()
	}
	r.peersWg.Wait()

	r.videoRelay.Close()
	r.audioRelay.Close()
}
