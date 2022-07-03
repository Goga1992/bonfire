#pragma once

#include "mixer_slot.hpp"

#include <gst/gst.h>

#include <memory>
#include <vector>

class VideoMixerRoom
{
 public:
  VideoMixerRoom(const RoomId room_id, const size_t num_speakers);

  VideoMixerRoom(const VideoMixerRoom&) = delete;
  VideoMixerRoom& operator=(const VideoMixerRoom&) = delete;

  ~VideoMixerRoom();

 public:
  void AddSlot(const SlotId slot_id, const std::optional<uint32_t> udpsrc_port, const SlotConfig& cfg);
  std::optional<uint32_t> RemoveSlot(const SlotId& slot_id);

  size_t Size() const { return slots.size(); }

 private:
  struct SlotConnection
  {
    std::unique_ptr<VideoMixerSlot> slot;

    GstPad* mixer_sink; // store sink so it can be released later
  };

 private:
  void DisconnectSlot(SlotConnection& slot_connection);

 private:
  RoomId id;

  GstElement* pipeline = nullptr;
  GstElement* mixer = nullptr;
  GstElement* multiudpsink = nullptr;

  std::vector<SlotConnection> slots;
};