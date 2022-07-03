#pragma once

#include "audiomixer_room.hpp"

#include <mutex>
#include <unordered_map>

class MixerManager
{
 public:
  MixerManager();
  ~MixerManager();

 public:
  SlotInfo AddSlot(const SlotConfig& cfg);
  void RemoveSlot(const SlotId& slot_id);

 private:
  std::mutex mtx;
  std::unordered_map<RoomId, std::unique_ptr<MixerRoom>> active_rooms;
  std::unordered_map<SlotId, RoomId> slot_to_room;
};