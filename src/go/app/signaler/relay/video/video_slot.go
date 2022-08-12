package video_relay

import (
	bon_log "bonlib/log"
	"context"
	"fmt"
	"io"
	"net"
	"os"
	"signaler/common"
	"signaler/transcode"
	"sync"
	"time"

	"sync/atomic"

	"signaler/relay/internal"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
)

type VideoSlot struct {
	mtx sync.RWMutex

	input  *internal.InputSender
	output map[string]*internal.OutputReceiver

	broadcasters map[string]*broadcaster

	incomingResolution atomic.Value

	subsCounter map[string]*int32
	subs        map[string]map[string]*Subscriber

	stopWorkers     atomic.Value
	inputWorkerDone chan any

	videoScalerClient   transcode.VideoScalerClient
	videoScalerSlotInfo *transcode.VideoSlotInfo
}

type broadcaster struct {
	resolution string
	workerDone chan any
}

func NewVideoSlot(track *webrtc.TrackRemote, videoScalerClient transcode.VideoScalerClient) (*VideoSlot, error) {
	outputReceivers := map[string]*internal.OutputReceiver{}
	subsCounter := map[string]*int32{}
	udpPorts := []uint32{}
	subs := map[string]map[string]*Subscriber{}

	// initialize receivers
	for _, resolution := range common.RESOLUTIONS {
		receiver, err := internal.NewOutputReceiver()
		if err != nil {
			return nil, err
		}
		outputReceivers[resolution.Name] = receiver
		udpPorts = append(udpPorts, uint32(receiver.UdpConn.LocalAddr().(*net.UDPAddr).Port))
		subsCounter[resolution.Name] = new(int32)
		subs[resolution.Name] = map[string]*Subscriber{}
	}

	// start VideoScaler slot
	bon_log.Info.Printf("Starting VideoScaler slot: track_id=[%s], udpPorts=[%v]", track.ID(), udpPorts)
	slotInfo, err := startVideoScalerSlot(videoScalerClient, udpPorts)
	if err != nil {
		return nil, err
	}
	bon_log.Info.Printf(
		"Got VideoScaler slot info: track_id=[%s], slot_id=[%s], hostname=[%s], port=[%d]",
		track.ID(), slotInfo.GetId(), slotInfo.GetHostname(), slotInfo.GetPort(),
	)

	// initialize sender
	inputSender, err := internal.NewInputSender(track, fmt.Sprintf("%s:%d", "127.0.0.1", slotInfo.GetPort()))
	if err != nil {
		return nil, err
	}

	slot := &VideoSlot{
		input:               inputSender,
		output:              outputReceivers,
		broadcasters:        map[string]*broadcaster{},
		incomingResolution:  atomic.Value{},
		subsCounter:         subsCounter,
		subs:                subs,
		stopWorkers:         atomic.Value{},
		inputWorkerDone:     make(chan any),
		videoScalerClient:   videoScalerClient,
		videoScalerSlotInfo: slotInfo,
	}

	slot.incomingResolution.Store(len(common.RESOLUTIONS))
	slot.stopWorkers.Store(false)

	return slot, nil
}

func startVideoScalerSlot(videoScalerClient transcode.VideoScalerClient, udpPorts []uint32) (*transcode.VideoSlotInfo, error) {
	// hostname, err := os.Hostname()
	// if err != nil {
	// 	return nil, err
	// }

	request := &transcode.StartVideoSlotRequest{
		SinkHostname: "127.0.0.1",
		SinkPorts:    udpPorts,
	}

	return videoScalerClient.StartVideoSlot(context.Background(), request)
}

func (slot *VideoSlot) stopVideoScalerSlot() error {
	request := &transcode.StopVideoSlotRequest{
		SlotId: slot.videoScalerSlotInfo.Id,
	}

	_, err := slot.videoScalerClient.StopVideoSlot(context.Background(), request)
	return err
}

func (slot *VideoSlot) Start() {
	for resolution_name, listener := range slot.output {
		go slot.broadcastWorker(&broadcaster{resolution_name, make(chan any)})
		go slot.outputWorker(listener, resolution_name)
	}

	go slot.inputWorker()
}

func (slot *VideoSlot) SetIncomingResolution(resolution string) {
	// Set incoming resolution
	slot.incomingResolution.Store(common.ResolutionIdx(resolution))

	for _, otherResolution := range common.RESOLUTIONS {
		// if other resolution has subs and is smaller then incoming then we need to transcode
		if atomic.LoadInt32(slot.subsCounter[otherResolution.Name]) > 0 &&
			common.ResolutionIdx(otherResolution.Name) > slot.incomingResolution.Load().(int) {
			slot.SetBranchActive(otherResolution.Name, true)
			continue
		}

		// no need to transcode otherwise
		slot.SetBranchActive(otherResolution.Name, false)
	}
}

func (slot *VideoSlot) ResolutionSubscribe(resolution string, peerID string) {
	slot.mtx.RLock()
	_, ok := slot.subs[resolution][peerID]
	slot.mtx.RUnlock()

	if ok {
		return
	}

	atomic.AddInt32(slot.subsCounter[resolution], 1)

	// if requested resolution is smaller then incoming then we need transcoding
	if common.ResolutionIdx(resolution) > slot.incomingResolution.Load().(int) {
		slot.SetBranchActive(resolution, true)
	}

	slot.mtx.Lock()
	slot.subs[resolution][peerID] = &Subscriber{peerID, make(chan *rtp.Packet, 200)}
	slot.mtx.Unlock()
}

func (slot *VideoSlot) ResolutionUnsubscribe(resolution string, peerID string) {
	subscribers := atomic.AddInt32(slot.subsCounter[resolution], -1)
	if subscribers < 0 {
		panic(fmt.Errorf("subscribers number should be positive"))
	}

	// if there are no subs to this resolution then no need to transcode
	if subscribers == 0 {
		slot.SetBranchActive(resolution, false)
	}

	slot.mtx.Lock()
	delete(slot.subs[resolution], peerID)
	slot.mtx.Unlock()
}

func (slot *VideoSlot) SetBranchActive(resolution string, active bool) {
	request := &transcode.SetSlotBranchActiveRequest{
		SlotId: slot.videoScalerSlotInfo.Id,
		Active: active,
		Name:   resolution,
	}

	slot.videoScalerClient.SetSlotBranchActive(context.Background(), request)
}

func (slot *VideoSlot) getTrack(resolution string) chan *rtp.Packet {
	return slot.output[resolution].PacketsChan
}

func (slot *VideoSlot) Stop() {
	slot.stopWorkers.Store(true)
	slot.Join()

	err := slot.input.UdpConn.Close()

	if err != nil {
		bon_log.Error.Printf("Could not close UDP connection: %s", err)
	}

	for _, listener := range slot.output {
		err := listener.UdpConn.Close()
		if err != nil {
			bon_log.Error.Printf("Could not close UDP connection: %s", err)
		}

		close(listener.PacketsChan)
	}

	bon_log.Info.Printf("Stopping VideoScaler slot: trackID=[%s]", slot.input.Track.ID())
	err = slot.stopVideoScalerSlot()
	if err != nil {
		bon_log.Error.Printf("Could not stop VideoScaler slot: %s", err)
	}
}

func (slot *VideoSlot) Join() {
	<-slot.inputWorkerDone

	for _, listener := range slot.output {
		<-listener.WorkerDone
	}

	for _, b := range slot.broadcasters {
		<-b.workerDone
	}

}

func (slot *VideoSlot) inputWorker() {
	defer func() {
		close(slot.inputWorkerDone)
	}()

	for !slot.stopWorkers.Load().(bool) {
		buffer := make([]byte, 1600)
		slot.input.Track.SetReadDeadline(time.Now().Add(time.Second))
		// packet, _, err := slot.input.track.ReadRTP()
		n, _, err := slot.input.Track.Read(buffer)
		if os.IsTimeout(err) {
			continue
		}

		if err == io.EOF {
			break
		}

		if err != nil {
			bon_log.Error.Printf("Could not read input from track: trackID=[%s]", slot.input.Track.ID())
			break
		}

		packet := &rtp.Packet{}
		packet.Unmarshal(buffer[:n])

		slot.input.LastTimestamp = internal.SetDiffTimestamp(packet, slot.input.LastTimestamp)

		needTranscoding := false
		for resolution, subscribers := range slot.subsCounter {
			// if resolution has no subscribers then we neither need to forward nor to transcode
			if atomic.LoadInt32(subscribers) == 0 {
				continue
			}

			// if resolution is bigger or equal to incoming resolution then we can forward it
			if common.ResolutionIdx(resolution) <= slot.incomingResolution.Load().(int) {
				slot.output[resolution].PacketsChan <- packet
				continue
			}

			// resolution has subs and we need to transcode it
			needTranscoding = true
		}

		if !needTranscoding {
			continue
		}

		_, err = slot.input.UdpConn.Write(buffer[:n])
		if err != nil {
			bon_log.Error.Printf("Could not write input to UDP: err=[%s], trackID=[%s]", err, slot.input.Track.ID())
			break
		}
	}
}

func (slot *VideoSlot) outputWorker(listener *internal.OutputReceiver, name string) {
	defer func() {
		close(listener.WorkerDone)
	}()

	for !slot.stopWorkers.Load().(bool) {
		buffer := make([]byte, 1600) // UDP MTU
		listener.UdpConn.SetReadDeadline(time.Now().Add(time.Second))
		n, _, err := listener.UdpConn.ReadFromUDP(buffer)
		if os.IsTimeout(err) {
			if atomic.LoadInt32(slot.subsCounter[name]) == 0 || common.ResolutionIdx(name) <= slot.incomingResolution.Load().(int) {
				continue
			}
			bon_log.Error.Printf("Could not read from output UDP: err=[%s], resolution=[%s], trackID=[%s]", err, name, slot.input.Track.ID())
			continue
		}

		if err != nil {
			bon_log.Error.Printf("Could not read from UDP: trackID=[%s], name=[%s]", slot.input.Track.ID(), name)
			break
		}

		var packet rtp.Packet
		packet.Unmarshal(buffer[:n])

		listener.LastTimestamp = internal.SetDiffTimestamp(&packet, listener.LastTimestamp)

		if common.ResolutionIdx(name) > slot.incomingResolution.Load().(int) {
			listener.PacketsChan <- &packet
		}
	}
}

func (s *VideoSlot) broadcastWorker(b *broadcaster) {
	defer close(b.workerDone)

	packetsChan := s.getTrack(b.resolution)

	for !s.stopWorkers.Load().(bool) {
		select {
		case packet, more := <-packetsChan:
			if !more {
				return
			}

			s.mtx.RLock()
			for _, subscriber := range s.subs[b.resolution] {
				// we don't wanna block because subscriber can't be trusted
				// he will unsubscribe eventually though
				select {
				case subscriber.sink <- packet:
				default:
				}
			}
			s.mtx.RUnlock()
		case <-b.workerDone:
			return
		}
	}
}

func (s *VideoSlot) GetPacketsChan(resolution string, peerID string) (chan *rtp.Packet, bool) {
	s.mtx.RLock()
	defer s.mtx.RUnlock()

	sink, ok := s.subs[resolution][peerID]
	if !ok {
		return nil, false
	}

	return sink.sink, true
}

func (s *VideoSlot) ForceKeyFrame(resolution string) error {
	request := &transcode.ForceKeyFrameRequest{
		SlotId: s.videoScalerSlotInfo.Id,
		Name:   resolution,
	}

	_, err := s.videoScalerClient.ForceKeyFrame(context.Background(), request)
	return err
}
