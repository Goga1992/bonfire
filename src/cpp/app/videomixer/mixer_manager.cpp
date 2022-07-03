#include "mixer_manager.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include <chrono>
#include <thread>

constexpr size_t NUM_SPEAKERS = 10;

VideoMixerManager::VideoMixerManager()
{
  const uint32_t start_port = bon::utils::GetEnvIntRequired("START_PORT");
  const uint32_t capacity = bon::utils::GetEnvIntRequired("CAPACITY");
  for (size_t i = start_port; i < start_port + capacity; ++i)
  {
    free_udp_ports.push_back(i);
  }
  // for easier testing
  std::reverse(free_udp_ports.begin(), free_udp_ports.end());
}

VideoMixerManager::~VideoMixerManager()
{
  std::scoped_lock lock(mtx);
}

SlotInfo VideoMixerManager::AddSlot(const SlotConfig& cfg)
{
  std::scoped_lock lock(mtx);

  // get udpsrc port if needed
  if (free_udp_ports.empty())
  {
    bon::log::Warn("Could not add slot, no free ports available");
    throw std::runtime_error("No free ports available");
  }

  auto udpsrc_port = free_udp_ports.back();
  free_udp_ports.pop_back();

  // create the room if not exists
  if (active_rooms.find(cfg.room_id) == active_rooms.end())
  {
    bon::log::Info("Opening room: room_id=[{}]", cfg.room_id);
    active_rooms.emplace(cfg.room_id, std::make_unique<VideoMixerRoom>(cfg.room_id, NUM_SPEAKERS));
  }

  // generate slot id
  SlotId slot_id = bon::utils::GenerateUUID();
  // for easier testing
  // slot_id.resize(4);

  // TODO: individual mutex
  bon::log::Info("Opening slot: room_id=[{}], slot_id=[{}]", cfg.room_id, slot_id);
  active_rooms[cfg.room_id]->AddSlot(slot_id, udpsrc_port, cfg);

  // map slot to room
  slot_to_room[slot_id] = cfg.room_id;

  return SlotInfo{slot_id, udpsrc_port};
}

void VideoMixerManager::RemoveSlot(const SlotId& slot_id)
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

  VideoMixerRoom* room = active_rooms[room_id].get();

  bon::log::Info("Closing slot: room_id=[{}], slot_id=[{}]", room_id, slot_id);
  const auto freed_port = room->RemoveSlot(slot_id);
  if (freed_port.has_value())
  {
    free_udp_ports.push_back(freed_port.value());
  }

  // close the room if it is the last slot
  if (room->Size() == 0)
  {
    bon::log::Info("Closing room: room_id=[{}]", room_id);
    active_rooms.erase(room_id);
  }
}