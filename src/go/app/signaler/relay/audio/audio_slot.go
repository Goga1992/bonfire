package audio_relay

import (
	bon_log "bonlib/log"
	"context"
	"fmt"
	"io"
	"net"
	"os"
	"signaler/relay/internal"
	"signaler/transcode"
	"time"

	"sync/atomic"

	"github.com/pion/rtp"
	"github.com/pion/webrtc/v3"
)

type AudioSlot struct {
	input  *internal.InputSender
	output *internal.OutputReceiver

	audioMixerClient transcode.AudioMixerClient
	audioMixerSlotID string

	stopWorkers      atomic.Value
	inputWorkerDone  chan any
	outputWorkerDone chan any
}

func NewAudioSlot(track *webrtc.TrackRemote, roomID string, audioMixerClient transcode.AudioMixerClient) (*AudioSlot, error) {
	outputReceiver, err := internal.NewOutputReceiver()
	if err != nil {
		return nil, err
	}

	resp, err := audioMixerClient.StartAudioSlot(
		context.Background(),
		&transcode.StartAudioSlotRequest{
			RoomId:       roomID,
			SinkHostname: "127.0.0.1",
			SinkPort:     uint32(outputReceiver.UdpConn.LocalAddr().(*net.UDPAddr).Port),
		},
	)
	if err != nil {
		return nil, fmt.Errorf("could not start audio slot: %s", err)
	}

	inputSender, err := internal.NewInputSender(track, fmt.Sprintf("%s:%d", "127.0.0.1", resp.GetPort()))
	if err != nil {
		return nil, err
	}

	bon_log.Debug.Println(resp.GetPort())

	slot := &AudioSlot{
		input:  inputSender,
		output: outputReceiver,

		audioMixerClient: audioMixerClient,
		audioMixerSlotID: resp.GetId(),

		stopWorkers:      atomic.Value{},
		inputWorkerDone:  make(chan any),
		outputWorkerDone: make(chan any),
	}

	slot.stopWorkers.Store(false)

	return slot, nil
}

func (slot *AudioSlot) Start() {
	go slot.outputWorker()
	go slot.inputWorker()
}

func (slot *AudioSlot) Stop() {
	slot.stopWorkers.Store(true)
	slot.Join()

	slot.audioMixerClient.StopAudioSlot(
		context.Background(),
		&transcode.StopAudioSlotRequest{
			SlotId: slot.audioMixerSlotID,
		},
	)
}

func (slot *AudioSlot) Join() {
	<-slot.inputWorkerDone
	<-slot.outputWorkerDone
}

func (slot *AudioSlot) GetTrack() chan *rtp.Packet {
	return slot.output.PacketsChan
}

func (slot *AudioSlot) inputWorker() {
	defer func() {
		close(slot.inputWorkerDone)
	}()

	for !slot.stopWorkers.Load().(bool) {
		buffer := make([]byte, 1600)
		slot.input.Track.SetReadDeadline(time.Now().Add(time.Second))
		n, _, err := slot.input.Track.Read(buffer)
		if os.IsTimeout(err) {
			bon_log.Error.Printf("Could not read input from track: trackID=[%s]", slot.input.Track.ID())
			continue
		}

		if err == io.EOF {
			break
		}

		if err != nil {
			bon_log.Error.Printf("Could not read input from track: trackID=[%s]", slot.input.Track.ID())
			break
		}

		_, err = slot.input.UdpConn.Write(buffer[:n])
		if err != nil {
			bon_log.Error.Printf("Could not write input to UDP: err=[%s], trackID=[%s]", err, slot.input.Track.ID())
			break
		}
	}
}

func (slot *AudioSlot) outputWorker() {
	defer func() {
		close(slot.outputWorkerDone)
	}()

	for !slot.stopWorkers.Load().(bool) {
		buffer := make([]byte, 1600) // UDP MTU
		slot.output.UdpConn.SetReadDeadline(time.Now().Add(time.Second))
		n, _, err := slot.output.UdpConn.ReadFromUDP(buffer)
		if os.IsTimeout(err) {
			bon_log.Error.Printf("Could not read from output UDP: err=[%s], trackID=[%s]", err, slot.input.Track.ID())
			continue
		}

		if err != nil {
			bon_log.Error.Printf("Could not read from UDP: trackID=[%s]", slot.input.Track.ID())
			break
		}

		var packet rtp.Packet
		packet.Unmarshal(buffer[:n])

		slot.output.LastTimestamp = internal.SetDiffTimestamp(&packet, slot.output.LastTimestamp)
		slot.output.PacketsChan <- &packet
	}
}
