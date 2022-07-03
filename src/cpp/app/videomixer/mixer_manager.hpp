#pragma once

#include "mixer_room.hpp"

#include <mutex>
#include <unordered_map>

class VideoMixerManager
{
 public:
  VideoMixerManager();
  ~VideoMixerManager();

 public:
  SlotInfo AddSlot(const SlotConfig& cfg);
  void RemoveSlot(const SlotId& slot_id);

 private:
  std::mutex mtx;
  std::unordered_map<RoomId, std::unique_ptr<VideoMixerRoom>> active_rooms;
  std::unordered_map<SlotId, RoomId> slot_to_room;
  std::vector<uint32_t> free_udp_ports;
};