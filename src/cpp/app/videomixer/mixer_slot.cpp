#include "mixer_slot.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include <fmt/format.h>

#include <random>

const size_t VideoMixerSlot::WIDTH = bon::utils::GetEnvIntRequired("WIDTH");
const size_t VideoMixerSlot::HEIGHT = bon::utils::GetEnvIntRequired("HEIGHT");

VideoMixerSlot::VideoMixerSlot(const SlotId slot_id,
                               const std::optional<uint32_t> udpsrc_port,
                               const SlotConfig& cfg)
: id(slot_id),
  udpsrc_port(udpsrc_port)
{
  InitDecoder(cfg);
}

// helper for constructing element
GstElement* VideoMixerSlot::MakeElement(GstElement* bin, std::string type, std::optional<std::string> name)
{
  auto element_id = name.value_or(type) + "_" + id;
  GstElement* element = gst_element_factory_make(type.c_str(), element_id.c_str());
  bon::utils::Assert(element != NULL, "Element should not be NULL");

  // transfers ownership to bin
  gst_bin_add(GST_BIN(bin), element);

  return element;
};

void VideoMixerSlot::InitDecoder(const SlotConfig& cfg)
{
  const auto make_element = [&](auto type, std::optional<std::string> name = std::nullopt)
  {
    return MakeElement(decoder_bin, type, name);
  };

  // create bin for easier element management
  decoder_bin = gst_bin_new(("decoder_bin_" + id).c_str());

  auto udpsrc = make_element("udpsrc");
  auto rtph264depay = make_element("rtph264depay");
  auto avdec_h264 = make_element("avdec_h264");
  auto videoscale = make_element("videoscale");
  auto videoscale_capsfilter = make_element("capsfilter");
  auto queue = make_element("queue");
  auto room_peer_element = queue;

  gst_element_link_many(udpsrc,
                        rtph264depay,
                        avdec_h264,
                        videoscale,
                        videoscale_capsfilter,
                        queue,
                        NULL);

  GstPad* room_peer_src = gst_element_get_static_pad(room_peer_element, "src");
  ghost_decoder_src = gst_ghost_pad_new("ghost_decoder_src", room_peer_src);
  gst_object_unref(room_peer_src);
  gst_element_add_pad(decoder_bin, ghost_decoder_src);

  // Set properties
  GstCaps* capsfilter_udpsrc = gst_caps_new_simple("application/x-rtp",
                                                   "clock-rate", G_TYPE_INT, 90000,
                                                   "payload", G_TYPE_INT, 96,
                                                   "encoding-name", G_TYPE_STRING, "H264",
                                                   NULL);
  g_object_set(udpsrc, "caps", capsfilter_udpsrc, NULL);
  gst_caps_unref(capsfilter_udpsrc);

  auto videoscale_caps = gst_caps_new_simple("video/x-raw",
                                             "width", G_TYPE_INT, WIDTH,
                                             "height", G_TYPE_INT, HEIGHT,
                                             NULL);
  g_object_set(videoscale_capsfilter, "caps", videoscale_caps, NULL);
  gst_caps_unref(videoscale_caps);

  g_object_set(udpsrc, "port", udpsrc_port.value_or(0), NULL);
}

void VideoMixerSlot::Reset()
{
  // set DecoderBin to NULL in order to free udp socket
  gst_element_set_state(decoder_bin, GST_STATE_NULL);

  udpsrc_port = std::nullopt;
}

VideoMixerSlot::~VideoMixerSlot()
{
  // Before
  gst_element_set_state(decoder_bin, GST_STATE_NULL);
  gst_object_unref(decoder_bin);
}