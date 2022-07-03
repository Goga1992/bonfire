#include "audiomixer_manager.hpp"

#include <logger/logger.hpp>
#include <utils/utils.hpp>

#include <chrono>
#include <thread>

constexpr size_t NUM_SPEAKERS = 10;

MixerManager::MixerManager()
{
}

MixerManager::~MixerManager()
{
  std::scoped_lock lock(mtx);
}

SlotInfo MixerManager::AddSlot(const SlotConfig& cfg)
{
  std::scoped_lock lock(mtx);

  // create the room if not exists
  if (active_rooms.find(cfg.room_id) == active_rooms.end())
  {
    bon::log::Info("Opening room: room_id=[{}]", cfg.room_id);
    active_rooms.emplace(cfg.room_id, std::make_unique<MixerRoom>(cfg.room_id, NUM_SPEAKERS));
  }

  // generate slot id
  SlotId slot_id = bon::utils::GenerateUUID();
  // for easier testing
  // slot_id.resize(4);

  // TODO: individual mutex
  bon::log::Info("Opening slot: room_id=[{}], slot_id=[{}]", cfg.room_id, slot_id);
  auto port = active_rooms[cfg.room_id]->AddSlot(slot_id, cfg);

  // map slot to room
  slot_to_room[slot_id] = cfg.room_id;

  return SlotInfo{slot_id, port.value_or(0)};
}

void MixerManager::RemoveSlot(const SlotId& slot_id)
{
  std::scoped_lock lock(mtx);

  const auto it = slot_to_room.find(slot_id);
  if (it == slot_to_room.end())
  {
    bon::log::Warn("Could not remove slot, found no room: slot_id=[{}]", slot_id);
    throw std::runtime_error("Slot does not belong to any room");
  }

  RoomId room_id = it->second;
  slot_to_room.erase(it);

  MixerRoom* room = active_rooms[room_id].get();

  bon::log::Info("Closing slot: room_id=[{}], slot_id=[{}]", room_id, slot_id);
  room->RemoveSlot(slot_id);

  // close the room if it is the last slot
  if (room->Size() == 0)
  {
    bon::log::Info("Closing room: room_id=[{}]", room_id);
    active_rooms.erase(room_id);
  }
}