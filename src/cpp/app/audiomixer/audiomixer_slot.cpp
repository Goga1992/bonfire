#include "audiomixer_slot.hpp"

#include <logger/logger.hpp>
#include <utils/utils.hpp>

#include <fmt/format.h>

#include <random>

const bool MixerSlot::SPATIAL_AUDIO = bon::utils::GetEnvIntWithDefault("SPATIAL_AUDIO", true);
const bool MixerSlot::NORMALIZE_AUDIO = bon::utils::GetEnvIntWithDefault("NORMALIZE_AUDIO", false);

MixerSlot::MixerSlot(const SlotId slot_id, const SlotConfig& cfg)
: id(slot_id),
  listener(cfg.listener)
{
  InitDecoder(cfg);
  InitEncoder(cfg);
}

// helper for constructing element
GstElement* MixerSlot::MakeElement(GstElement* bin, std::string type, std::optional<std::string> name)
{
  auto element_id = name.value_or(type) + "_" + id;
  GstElement* element = gst_element_factory_make(type.c_str(), element_id.c_str());
  bon::utils::Assert(element != NULL, "Element should not be NULL");

  // transfers ownership to bin
  gst_bin_add(GST_BIN(bin), element);

  return element;
};

// udpsrc ! queue_rtp_in ! rtpopusdepay ! opusdec ! audioconvert_in ! audioresample ! capsfilter ! audiopanorama
void MixerSlot::InitDecoder(const SlotConfig& cfg)
{
  const auto make_element = [&](auto type, std::optional<std::string> name = std::nullopt)
  {
    return MakeElement(decoder_bin, type, name);
  };

  // create bin for easier element management
  decoder_bin = gst_bin_new(("decoder_bin_" + id).c_str());

  // listener slots do not produce any audio,
  // so there is no need to allocate decoding/processing elements
  if (Listener())
  {
    // dummy element just to avoid extra handling during connect/disconnect
    auto valve = make_element("valve");
    g_object_set(valve, "drop", true, NULL);

    // create ghost pad to link outside bin
    GstPad* room_peer_src = gst_element_get_static_pad(valve, "src");
    ghost_decoder_src = gst_ghost_pad_new("ghost_decoder_src", room_peer_src);
    gst_object_unref(room_peer_src);
    gst_element_add_pad(decoder_bin, ghost_decoder_src); // transfers ownership to bin

    return;
  }

  // create elements
  udpsrc = make_element("udpsrc");
  auto queue_rtp_in = make_element("identity", "queue_rtp_in");
  auto rtpopusdepay = make_element("rtpopusdepay");
  auto opusdec = make_element("opusdec");
  auto audioconvert_in = make_element("audioconvert", "audioconvert_in");
  auto audioresample = make_element("audioresample");
  auto capsfilter_src = make_element("capsfilter");
  auto room_peer_element = capsfilter_src;

  // link elements before room mixer
  gst_element_link_many(udpsrc.value(),
                        queue_rtp_in,
                        rtpopusdepay,
                        opusdec,
                        audioconvert_in,
                        audioresample,
                        capsfilter_src,
                        NULL);

  if (SPATIAL_AUDIO)
  {
    // choose random panorama value
    static constexpr float panorama[] = {-0.6, -0.5, -0.3, 0, 0.3, 0.5, 0.6};
    std::random_device dev;
    std::mt19937 generator(dev());
    std::uniform_int_distribution<int> distribution(0, std::size(panorama)-1);
    const float pan = panorama[distribution(generator)];

    auto audiopanorama = make_element("audiopanorama", "panorama");
    g_object_set(audiopanorama, "panorama", pan, NULL);
    g_object_set(audiopanorama, "method", 1, NULL); // use simple method, psychoacoustic produces noisy audio

    gst_element_link(capsfilter_src, audiopanorama);

    room_peer_element = audiopanorama;
  }

  // create ghost pad to link outside bin
  GstPad* room_peer_src = gst_element_get_static_pad(room_peer_element, "src");
  ghost_decoder_src = gst_ghost_pad_new("ghost_decoder_src", room_peer_src);
  gst_object_unref(room_peer_src);
  gst_element_add_pad(decoder_bin, ghost_decoder_src); // transfers ownership to bin

  // Set properties
  GstCaps* capsfilter_udpsrc = gst_caps_new_simple("application/x-rtp",
                                                   "clock-rate", G_TYPE_INT, 48000,
                                                   "payload", G_TYPE_INT, 96,
                                                   "encoding-name", G_TYPE_STRING, "OPUS",
                                                   NULL);
  g_object_set(udpsrc.value(), "caps", capsfilter_udpsrc, NULL);
  gst_caps_unref(capsfilter_udpsrc);

  GstCaps* capsfilter_src_caps = gst_caps_new_simple("audio/x-raw", "format", G_TYPE_STRING, "S16LE", NULL);
  g_object_set(capsfilter_src, "caps", capsfilter_src_caps, NULL);
  gst_caps_unref(capsfilter_src_caps);

  g_object_set(udpsrc.value(), "port", 0, NULL);
}

// room_tee.src ! room_queue ! ... normalize ... ! opusenc ! rtpopuspay ! capsfilter ! udpsink
void MixerSlot::InitEncoder(const SlotConfig& cfg)
{
  const auto make_element = [&](auto type, std::optional<std::string> name = std::nullopt)
  {
    return MakeElement(encoder_bin, type, name);
  };

  // create bin for easier element management
  encoder_bin = gst_bin_new(("encoder_bin_" + id).c_str());

  auto room_queue = make_element("queue", "room_queue");
  auto opusenc_peer_element = room_queue;

  // https://coaxion.net/blog/2020/07/live-loudness-normalization-in-gstreamer-experiences-with-porting-a-c-audio-filter-to-rust/
  // note that due to its nature rsaudioloudnorm has initial 3 seconds latency
  // there are also audio dropouts sometimes ?
  if (NORMALIZE_AUDIO)
  {
    auto audioresample_out1 = make_element("audioresample", "audioresample_out1");
    auto audioconvert_out1 = make_element("audioconvert", "audioconvert_out1");
    auto rsaudioloudnorm = make_element("rsaudioloudnorm");
    auto audioresample_out2 = make_element("audioresample", "audioresample_out2");
    auto audioconvert_out2 = make_element("audioconvert", "audioconvert_out2");

    gst_element_link_many(room_queue,
                          audioconvert_out1,
                          audioresample_out1,
                          rsaudioloudnorm,
                          audioconvert_out2,
                          audioresample_out2,
                          NULL);

    opusenc_peer_element = audioresample_out2;
  }

  auto opusenc = make_element("opusenc");
  auto rtpopuspay = make_element("rtpopuspay");
  auto capsfilter_sink = make_element("capsfilter", "capsfilter_sink");
  udpsink = make_element("udpsink");

  gst_element_link_many(opusenc_peer_element,
                        opusenc,
                        rtpopuspay,
                        capsfilter_sink,
                        udpsink,
                        NULL);

  // request sink pad for and create ghost pad to link outside bin
  GstPad* room_queue_sink = gst_element_get_static_pad(room_queue, "sink");
  ghost_encoder_sink = gst_ghost_pad_new("ghost_encoder_sink", room_queue_sink);
  gst_object_unref(room_queue_sink);
  gst_element_add_pad(encoder_bin, ghost_encoder_sink); // transfers ownership to bin

  // seems like small sizes generally work better
  g_object_set(room_queue, "max-size-buffers", 1, NULL);
  g_object_set(room_queue, "silent", true, NULL);

  GstCaps* capsfilter_sink_caps = gst_caps_new_empty_simple("application/x-rtp");
  g_object_set(capsfilter_sink, "caps", capsfilter_sink_caps, NULL);
  gst_caps_unref(capsfilter_sink_caps);

  g_object_set(udpsink,
               "clients", fmt::format("{}:{}", cfg.sink_hostname, cfg.sink_port).c_str(),
               "async", false,
               "sync", false, NULL);
}

std::optional<uint32_t> MixerSlot::UdpsrcPort() const
{
  if (not udpsrc.has_value())
  {
    return std::nullopt;
  }

  guint32 udpsrc_port;
  g_object_get(udpsrc.value(), "port", &udpsrc_port, NULL);
  return udpsrc_port;
}

void MixerSlot::Reset()
{
  // set DecoderBin to NULL in order to free udp socket
  gst_element_set_state(decoder_bin, GST_STATE_NULL);

  // redirect output
  g_object_set(udpsink, "clients", "", NULL);
}

MixerSlot::~MixerSlot()
{
  // Before
  gst_element_set_state(decoder_bin, GST_STATE_NULL);
  gst_object_unref(decoder_bin);

  // After
  gst_element_set_state(encoder_bin, GST_STATE_NULL);
  gst_object_unref(encoder_bin);
}