#include "videoscaler_manager.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include <chrono>
#include <thread>

constexpr size_t NUM_SPEAKERS = 10;

VideoScalerManager::VideoScalerManager()
: capacity(bon::utils::GetEnvIntRequired("CAPACITY"))
{
}

VideoScalerManager::~VideoScalerManager()
{
}

SlotInfo VideoScalerManager::StartVideoSlot(const SlotConfig& cfg)
{
  std::scoped_lock lock(mtx);

  // get udpsrc port if needed
  if (slots.size() == capacity)
  {
    bon::log::Warn("Could not start slot, maximum capacity reached");
    throw std::runtime_error("maximum capacity reached");
  }

  // generate slot id
  SlotId slot_id = bon::utils::GenerateUUID();
  // for easier testing
  // slot_id.resize(4);

  bon::log::Info("Starting slot: slot_id=[{}]", slot_id);
  slots[slot_id] = std::make_unique<VideoScalerSlot>(slot_id, cfg);

  return SlotInfo{slot_id, slots[slot_id]->UdpsrcPort()};
}

void VideoScalerManager::SetSlotBranchActive(const SlotId& slot_id,
                                             const std::string& name,
                                             bool active)
{
  std::scoped_lock lock(mtx);

  const auto it = slots.find(slot_id);
  if (it == slots.end())
  {
    bon::log::Warn("Could not find slot: slot_id=[{}]", slot_id);
    throw std::runtime_error("slot not found");
  }

  it->second->SetBranchActive(name, active);
}

void VideoScalerManager::ForceKeyFrame(const SlotId& slot_id, const std::string& name)
{
  std::scoped_lock lock(mtx);

  const auto it = slots.find(slot_id);
  if (it == slots.end())
  {
    bon::log::Warn("Could not find slot: slot_id=[{}]", slot_id);
    throw std::runtime_error("slot not found");
  }

  it->second->ForceKeyFrame(name);
}

void VideoScalerManager::StopVideoSlot(const SlotId& slot_id)
{
  std::scoped_lock lock(mtx);

  const auto it = slots.find(slot_id);
  if (it == slots.end())
  {
    bon::log::Warn("Could not find slot: slot_id=[{}]", slot_id);
    throw std::runtime_error("slot not found");
  }

  bon::log::Info("Stopping slot: slot_id=[{}]", slot_id);
  VideoScalerSlot& slot = *(it->second);
  slots.erase(it);
}