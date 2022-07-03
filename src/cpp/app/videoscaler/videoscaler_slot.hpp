#pragma once

#include "types.hpp"

#include <gst/gst.h>

#include <atomic>

class VideoScalerSlot
{
 public:
  static constexpr VideoResolution RESOLUTIONS[] =
  {
    {"High", 640, 480},
    {"Medium", 480, 360},
    {"Low", 320, 240}
  };

 public:
  VideoScalerSlot(const SlotId slot_id, const SlotConfig& cfg);

  VideoScalerSlot(const VideoScalerSlot&) = delete;
  VideoScalerSlot& operator=(const VideoScalerSlot&) = delete;

  ~VideoScalerSlot();

 public:
  void SetBranchActive(const std::string& name, bool active);
  void ForceKeyFrame(const std::string& name);

  const SlotId& Id() const { return id; }
  const uint32_t UdpsrcPort() const { return udpsrc_port; }

 private:
  struct VideoCaps
  {
    int width = 0;
    int height = 0;
    std::pair<int, int> pixel_aspect_ratio = {0, 0};

    bool operator==(const VideoCaps& other)
    {
      return width == other.width &&
             height == other.height &&
             pixel_aspect_ratio == other.pixel_aspect_ratio;
    }
  };

  struct CapsProbeData
  {
    VideoCaps last_caps;
    std::vector<std::pair<std::string, GstElement*>> branch_caps_filters;
  };

 private:
  GstElement* MakeElement(std::string type, std::optional<std::string> name = std::nullopt);
  void InitDecoder();
  void InitBranch(std::string_view branch_name, const std::string& sink_hostname, uint32_t sink_port);

  static GstPadProbeReturn CapsProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
  static std::pair<float, float> GetDowngradeFactor(size_t res_idx_0, size_t res_idx_1);
  static size_t MatchCapsToBranch(const VideoCaps& caps);

 private:
  SlotId id;
  uint32_t udpsrc_port;

  GstElement* pipeline = nullptr;
  GstElement* udpsrc = nullptr;
  GstElement* tee = nullptr;

  std::vector<GstPad*> tee_srcs;
  std::vector<std::pair<std::string, GstElement*>> branch_valves;
  std::vector<std::pair<std::string, GstElement*>> branch_encoders;

  CapsProbeData caps_probe_data;

  std::atomic<size_t> num_active_branches = 0;
};