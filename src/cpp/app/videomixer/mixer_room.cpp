#include "mixer_room.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include <algorithm>
#include <thread>

VideoMixerRoom::VideoMixerRoom(const RoomId room_id, const size_t num_speakers) : id(room_id)
{
  const auto make_element = [&](std::string type, std::optional<std::string> name = std::nullopt)
  {
    auto element_id = name.value_or(type) + "_" + id;
    GstElement* element = gst_element_factory_make(type.c_str(), element_id.c_str());
    bon::utils::Assert(element != NULL, "Element should not be NULL");

    // transfers ownership to bin
    gst_bin_add(GST_BIN(pipeline), element);

    return element;
  };

  pipeline = gst_pipeline_new(id.c_str());

  mixer = make_element("compositor");
  auto x264enc = make_element("x264enc");
  auto rtph264pay = make_element("rtph264pay");
  multiudpsink = make_element("multiudpsink");

  g_object_set(mixer, "ignore-inactive-pads", true, "start-time-selection", 1, NULL);
  g_object_set(multiudpsink, "async", false, NULL);

  gst_element_link_many(mixer,
                        x264enc,
                        rtph264pay,
                        multiudpsink,
                        NULL);

  auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  bon::utils::Assert(ret == GST_STATE_CHANGE_SUCCESS, "State change should be GST_STATE_CHANGE_SUCCESS");
}

VideoMixerRoom::~VideoMixerRoom()
{
  // set pipeline to NULL
  gst_element_set_state(pipeline, GST_STATE_NULL);

  // free remaining slot connections
  for (auto& sc : slots)
  {
    DisconnectSlot(sc);
  }
  slots.clear();

  gst_object_unref(pipeline);
}

void VideoMixerRoom::AddSlot(const SlotId slot_id, const std::optional<uint32_t> udpsrc_port, const SlotConfig& cfg)
{
  // create slot
  auto slot = std::make_unique<VideoMixerSlot>(slot_id, udpsrc_port, cfg);

  // add slot bins to pipeline
  gst_bin_add(GST_BIN(pipeline), slot->DecoderBin());

  // request audiomixer.sink and link slot to it
  GstPad* mixer_sink = gst_element_request_pad_simple(mixer, "sink_%u"); // unref in DisconnectSlot
  gst_pad_link(slot->DecoderSrc(), mixer_sink);
  g_object_set(mixer_sink, "xpos", VideoMixerSlot::WIDTH * Size(), NULL);

  gst_element_sync_state_with_parent(slot->DecoderBin());

  g_signal_emit_by_name(multiudpsink, "add", cfg.callback_hostname.c_str(), cfg.callback_port);

  gst_debug_bin_to_dot_file_with_ts(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "VideoMixerRoom_graph");

  slots.push_back({std::move(slot), mixer_sink});
}

std::optional<uint32_t> VideoMixerRoom::RemoveSlot(const SlotId& slot_id)
{
  // find slot by id
  auto it = std::find_if(slots.begin(), slots.end(),
                         [&slot_id](const SlotConnection& sc) { return sc.slot->Id() == slot_id; });

  if (it == slots.end())
  {
    bon::log::Warn("Could not remove slot, no slot with such id: slot_id=[{}]", slot_id);
    throw std::runtime_error("No slot with such id");
  }

  // reset udp ports
  const auto udpsrc_port = it->slot->UdpsrcPort();
  it->slot->Reset();

  DisconnectSlot(*it);

  slots.erase(it);

  return udpsrc_port;
}

void VideoMixerRoom::DisconnectSlot(SlotConnection& slot_connection)
{
  auto& [slot, mixer_sink] = slot_connection;

  // State changes to GST_STATE_READY or GST_STATE_NULL never return GST_STATE_CHANGE_ASYNC.
  gst_element_set_state(slot->DecoderBin(), GST_STATE_NULL);

  // unlink and release mixer sink
  gst_pad_unlink(slot->DecoderSrc(), mixer_sink);
  gst_element_release_request_pad(mixer, mixer_sink);
  gst_object_unref(mixer_sink);

  // removing slot from pipeline will drop refcount to zero
  // but we may want to reuse slot in the future,
  // we also still need to free its resources so we increment refcount
  gst_object_ref(slot->DecoderBin());
  gst_bin_remove(GST_BIN(pipeline), slot->DecoderBin());
}