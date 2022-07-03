#pragma once

#include "types.hpp"
#include "videoscaler_slot.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>

class VideoScalerManager
{
 public:
  VideoScalerManager();
  ~VideoScalerManager();

 public:
  SlotInfo StartVideoSlot(const SlotConfig& cfg);

  void SetSlotBranchActive(const SlotId& slot_id,
                           const std::string& name,
                           bool active);

  void ForceKeyFrame(const SlotId& slot_id, const std::string& name);

  void StopVideoSlot(const SlotId& slot_id);

 private:
  std::mutex mtx;
  std::unordered_map<SlotId, std::unique_ptr<VideoScalerSlot>> slots;
  size_t capacity;
};