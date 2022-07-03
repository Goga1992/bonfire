package peer

import (
	bon_log "bonlib/log"
	"encoding/json"
	"fmt"
	"signaler/common"

	"github.com/pion/webrtc/v3"
)

type resolutionChangedChannel struct {
	src  *webrtc.DataChannel
	sink chan string
}

type resolutionWantedChannel struct {
	src  *webrtc.DataChannel
	sink chan map[string]string
}

type participantsChangeChannel struct {
	src  chan []string
	sink *webrtc.DataChannel
}

func (p *Peer) newResolutionChangedChannel() (*resolutionChangedChannel, error) {
	sinkChannel := make(chan string)

	srcChannel, err := p.conn.CreateDataChannel("resolution_changed_"+p.id, nil)
	if err != nil {
		return nil, fmt.Errorf("could not create resolution_changed datachannel: %w", err)
	}

	srcChannel.OnOpen(func() {
		bon_log.Info.Printf("DataChannel opened: label=[%s], id=[%d]", srcChannel.Label(), srcChannel.ID())
	})

	srcChannel.OnMessage(func(msg webrtc.DataChannelMessage) {
		var stats map[string]int
		json.Unmarshal(msg.Data, &stats)
		bon_log.Info.Printf("Message from [%s]: %v", srcChannel.Label(), stats)

		idx := common.MatchCapsToBranch(stats["width"], stats["height"])
		sinkChannel <- common.RESOLUTIONS[idx].Name
	})

	srcChannel.OnClose(func() { close(sinkChannel) })

	return &resolutionChangedChannel{srcChannel, sinkChannel}, nil
}

func (c *resolutionChangedChannel) Close() {
	err := c.src.Close()
	if err != nil {
		bon_log.Error.Printf("Could not close resolution_changed datachannel: %s", err)
	}
	bon_log.Info.Println("RESOLUTION CHANGED CLOSED")
}

func (c *resolutionChangedChannel) Sink() chan string {
	return c.sink
}

func (p *Peer) newResolutionWantedChannel() (*resolutionWantedChannel, error) {
	sinkChannel := make(chan map[string]string)

	srcChannel, err := p.conn.CreateDataChannel("resolution_wanted_"+p.id, nil)
	if err != nil {
		return nil, fmt.Errorf("could not create resolution_wanted datachannel: %w", err)
	}

	srcChannel.OnOpen(func() {
		bon_log.Info.Printf("DataChannel opened: label=[%s], id=[%d]", srcChannel.Label(), srcChannel.ID())
	})

	srcChannel.OnMessage(func(msg webrtc.DataChannelMessage) {
		var wanted map[string]string
		json.Unmarshal(msg.Data, &wanted)
		bon_log.Info.Printf("Message from [%s]: %v", srcChannel.Label(), wanted)

		sinkChannel <- wanted
	})

	srcChannel.OnClose(func() { close(sinkChannel) })

	return &resolutionWantedChannel{srcChannel, sinkChannel}, nil
}

func (c *resolutionWantedChannel) Sink() chan map[string]string {
	return c.sink
}

func (c *resolutionWantedChannel) Close() {
	err := c.src.Close()
	if err != nil {
		bon_log.Error.Printf("Could not close resolution_wanted datachannel: %s", err)
	}
	bon_log.Info.Println("RESOLUTION CLOSED")
}

func (p *Peer) newParticipantsChangeChannel() (*participantsChangeChannel, error) {
	srcChannel := make(chan []string)

	sinkChannel, err := p.conn.CreateDataChannel("participants_change_"+p.id, nil)
	if err != nil {
		return nil, fmt.Errorf("could not create resolution_wanted datachannel: %w", err)
	}

	sinkChannel.OnOpen(func() {
		bon_log.Info.Printf("DataChannel opened: label=[%s], id=[%d]", sinkChannel.Label(), sinkChannel.ID())

		for {
			change, more := <-srcChannel
			if !more {
				return
			}

			data, err := json.Marshal(change)
			if err != nil {
				bon_log.Error.Printf("Could not marshal change data: %s", err)
				continue
			}
			bon_log.Debug.Println(string(data))

			err = sinkChannel.Send(data)
			if err != nil {
				bon_log.Error.Printf("Could not send change data: %s", err)
			}
		}
	})

	return &participantsChangeChannel{srcChannel, sinkChannel}, nil
}

func (c *participantsChangeChannel) Src() chan []string {
	return c.src
}

func (c *participantsChangeChannel) Close() {
	err := c.sink.Close()
	if err != nil {
		bon_log.Error.Printf("Could not close resolution_wanted datachannel: %s", err)
	}
}
