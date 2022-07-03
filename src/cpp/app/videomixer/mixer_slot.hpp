#pragma once

#include "types.hpp"

#include <gst/gst.h>

class VideoMixerSlot
{
 public:
  static const size_t WIDTH;
  static const size_t HEIGHT;

 public:
  VideoMixerSlot(const SlotId slot_id, const std::optional<uint32_t> udpsrc_port, const SlotConfig& cfg);

  VideoMixerSlot(const VideoMixerSlot&) = delete;
  VideoMixerSlot& operator=(const VideoMixerSlot&) = delete;

  ~VideoMixerSlot();

 public:
  void Reset();

  const SlotId& Id() const { return id; }
  const std::optional<uint32_t> UdpsrcPort() const { return udpsrc_port; }

  GstPad* DecoderSrc() { return ghost_decoder_src; }
  GstElement* DecoderBin() { return decoder_bin; }

 private:
  GstElement* MakeElement(GstElement* bin, std::string type, std::optional<std::string> name = std::nullopt);
  void InitDecoder(const SlotConfig& cfg);

 private:
  SlotId id;
  std::optional<uint32_t> udpsrc_port = std::nullopt;

  GstElement* decoder_bin = nullptr;
  GstElement* udpsrc = nullptr;
  GstPad* ghost_decoder_src = nullptr;
};