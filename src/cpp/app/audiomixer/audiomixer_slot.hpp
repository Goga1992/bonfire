#pragma once

#include "types.hpp"

#include <gst/gst.h>

#include <optional>

class MixerSlot
{
 public:
  static const bool SPATIAL_AUDIO;
  static const bool NORMALIZE_AUDIO;

 public:
  MixerSlot(const SlotId slot_id, const SlotConfig& cfg);

  MixerSlot(const MixerSlot&) = delete;
  MixerSlot& operator=(const MixerSlot&) = delete;

  ~MixerSlot();

 public:
  void Reset();

  const SlotId& Id() const { return id; }
  std::optional<uint32_t> UdpsrcPort() const;
  bool Listener() const { return listener; }

  GstPad* DecoderSrc() { return ghost_decoder_src; }
  GstPad* EncoderSink() { return ghost_encoder_sink; }
  GstElement* DecoderBin() { return decoder_bin; }
  GstElement* EncoderBin() { return encoder_bin; }

 private:
  GstElement* MakeElement(GstElement* bin, std::string type, std::optional<std::string> name = std::nullopt);
  void InitDecoder(const SlotConfig& cfg);
  void InitEncoder(const SlotConfig& cfg);

 private:
  SlotId id;
  bool listener;

  GstElement* decoder_bin = nullptr;
  std::optional<GstElement*> udpsrc;
  GstPad* ghost_decoder_src = nullptr;

  GstElement* encoder_bin = nullptr;
  GstElement* udpsink = nullptr;
  GstPad* ghost_encoder_sink = nullptr;
};