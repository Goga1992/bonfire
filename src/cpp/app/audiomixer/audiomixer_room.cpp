#include "audiomixer_room.hpp"
#include "gst_mix_input_meta.hpp"
#include "gst-plugins/bonaudiomixer/gstaudiomixerorc.h"
#include "gst-plugins/bonaudiomixer/gstaudiomixer.h"

#include <logger/logger.hpp>
#include <utils/utils.hpp>

#include <algorithm>
#include <thread>

MixerRoom::MixerRoom(const RoomId room_id, const size_t num_speakers) : id(room_id)
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

  mixer = make_element("bonaudiomixer", "mixer");
  auto capsfilter_mixer = make_element("capsfilter", "capsfilter_mixer");
  tee = make_element("tee", "mixer_tee");
  gst_element_link_many(mixer, capsfilter_mixer, tee, NULL);

  // allow tee to have no src pads
  g_object_set(tee, "allow-not-linked", true, NULL);
  g_object_set(mixer, "start-time-selection", 1, NULL); // IIUC this sets outbuf time to time of first inbuf to arrive
  g_object_set(mixer, "ignore-inactive-pads", true, NULL);

  GstCaps* caps = gst_caps_new_simple("audio/x-raw",
                                      "format", G_TYPE_STRING, "S16LE",
                                      "rate", G_TYPE_INT, 48000,
                                      "channels", G_TYPE_INT, MixerSlot::SPATIAL_AUDIO ? 2 : 1,
                                      NULL);
  g_object_set(capsfilter_mixer, "caps", caps, NULL);
  gst_caps_unref(caps);

  auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  bon::utils::Assert(ret == GST_STATE_CHANGE_SUCCESS, "State change should be GST_STATE_CHANGE_SUCCESS");
}

MixerRoom::~MixerRoom()
{
  // set pipeline to NULL
  gst_element_set_state(pipeline, GST_STATE_NULL);

  // there may still be slots waiting for deallocation
  while (SlotsToBeDeleted() > 0)
  {
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  }

  // free remaining slot connections
  for (auto& sc : slots)
  {
    DisconnectSlot(sc);
  }
  slots.clear();

  gst_object_unref(pipeline);
}

void MixerRoom::SampleCallbackFunc(gpointer user_data,
                                   GstPad* pad,
                                   GstBuffer* inbuf,
                                   GstBuffer* outbuf,
                                   guint in_offset,
                                   guint out_offset,
                                   guint num_frames,
                                   gint bpf,
                                   gint channels)
{
  // we need to remember user's own contribution to mix
  // in order to subtract it later in MixMinusProbe
  gst_buffer_add_mix_input_meta(outbuf, SampleFromPad{pad, inbuf, in_offset, out_offset, num_frames, bpf, channels});
}

// example of modifying buffer contents (snippet in "Data probes" section):
// https://gstreamer.freedesktop.org/documentation/application-development/advanced/pipeline-manipulation.html?gi-language=c#:~:text=static%20GstPadProbeReturn%0Acb_have_data,buffer%3B%0A%0A%20%20return%20GST_PAD_PROBE_OK%3B%0A%7D
GstPadProbeReturn MixerRoom::MixMinusProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
  // corresponding mixer.sink pad
  GstPad* sink_pad = static_cast<GstPad*>(user_data);

  // tee does not copy buffer for each of its src pads
  // so we have to copy to make it writable
  // we do not deep copy because listener pads don't need to write (but they do when removing meta from outbuf)))
  GstBuffer* outbuf_old = GST_PAD_PROBE_INFO_BUFFER(info);
  GstBuffer* outbuf = gst_buffer_copy(outbuf_old);
  GST_PAD_PROBE_INFO_DATA(info) = outbuf;
  gst_buffer_unref(outbuf_old);

  // iterate over each buffer meta
  // single buffer meta corresponds to some input buffer from mixer.sink pad
  // which was mixed into outbuf
  gpointer state = NULL;
  std::vector<GstMeta*> to_delete;
  to_delete.reserve(2*10); // 2 channels and 10 speakers
  while (GstMeta* cur_meta = gst_buffer_iterate_meta_filtered(outbuf, &state, GST_MIX_INPUT_META_API_TYPE))
  {
    to_delete.push_back(cur_meta);

    auto mix_input_meta = reinterpret_cast<GstMixInputMeta*>(cur_meta);
    auto [sample_pad, inbuf, in_offset, out_offset, num_frames, bpf, channels] = mix_input_meta->sample;

    if (sample_pad != sink_pad)
    {
      continue;
    }

    // TODO: optimize ? map outbuf only once
    GstMapInfo inmap;
    GstMapInfo outmap;

    if (not gst_buffer_map(outbuf, &outmap, GST_MAP_READWRITE))
    {
      bon::log::Warn("Cannot map outbuf");
      continue;
    }

    if (not gst_buffer_map(inbuf, &inmap, GST_MAP_READ))
    {
      bon::log::Warn("Cannot map inbuf");
      gst_buffer_unmap(outbuf, &outmap);
      continue;
    }

    audiomixer_orc_sub_s16(reinterpret_cast<gint16*>(outmap.data + out_offset * bpf),
                           reinterpret_cast<gint16*>(inmap.data + in_offset * bpf),
                           num_frames * channels);

    gst_buffer_unmap(inbuf, &inmap);
    gst_buffer_unmap(outbuf, &outmap);
  }

  // removing meta manually is not necessary,
  // we just do it to free resources as soon as possible,
  // otherwise meta will be freed later with buffer itself
  for (auto meta : to_delete)
  {
    gst_buffer_remove_meta(outbuf, meta);
  }

  return GST_PAD_PROBE_OK;
}

std::optional<uint32_t> MixerRoom::AddSlot(const SlotId slot_id, const SlotConfig& cfg)
{
  // create slot
  auto slot = std::make_unique<MixerSlot>(slot_id, cfg);

  // add slot bins to pipeline
  gst_bin_add(GST_BIN(pipeline), slot->DecoderBin());
  gst_bin_add(GST_BIN(pipeline), slot->EncoderBin());

  // request audiomixer.sink and link slot to it
  GstPad* mixer_sink = gst_element_request_pad_simple(mixer, "sink_%u"); // unref in DisconnectSlot
  gst_pad_link(slot->DecoderSrc(), mixer_sink);
  g_object_set(mixer_sink, "sample-callback-func", static_cast<GstSampleCallbackFunc>(SampleCallbackFunc), NULL);
  if (slot->Listener())
  {
    g_object_set(mixer_sink, "mute", true, NULL);
  }

  // note that slot->EncoderBin should be synced with room BEFORE linking tee_src to slot->EncoderSink,
  // otherwise there is dataflow from tee_src but slot->EncoderBin is still in NULL state
  // and will return an error when it receives buffer
  gst_element_sync_state_with_parent(slot->EncoderBin());

  // request tee.src and link slot to it
  // TODO: maybe add queue
  GstPad* tee_src = gst_element_request_pad_simple(tee, "src_%u"); // unref in DisconnectSlot
  gst_pad_link(tee_src, slot->EncoderSink());
  gst_pad_add_probe(tee_src, GST_PAD_PROBE_TYPE_BUFFER, static_cast<GstPadProbeCallback>(MixMinusProbe), mixer_sink, NULL);

  gst_element_sync_state_with_parent(slot->DecoderBin());

  gst_debug_bin_to_dot_file_with_ts(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline_graph");

  auto udpsrc_port = slot->UdpsrcPort();
  slots.push_back({std::move(slot), mixer_sink, tee_src});

  return udpsrc_port;
}

void MixerRoom::RemoveSlot(const SlotId& slot_id)
{
  // find slot by id
  auto it = std::find_if(slots.begin(), slots.end(),
                         [&slot_id](const SlotConnection& sc) { return sc.slot->Id() == slot_id; });

  if (it == slots.end())
  {
    bon::log::Warn("Could not remove slot, no slot with such id: slot_id=[{}]", slot_id);
    throw std::runtime_error("No slot with such id");
  }

  // because we can't unlink till the tee_src pad is idle
  // we have to detach slot connection and free its resources asynchronously later
  auto data = new DisconnectSlotProbeData{std::move(*it), *this};
  slots.erase(it);

  ++slots_to_be_deleted;
  gst_pad_add_probe(data->slot_connection.tee_src, GST_PAD_PROBE_TYPE_IDLE, DisconnectSlotWhenIdleProbe, data, NULL);
}

GstPadProbeReturn MixerRoom::DisconnectSlotWhenIdleProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
  auto data = static_cast<DisconnectSlotProbeData*>(user_data);

  data->room.DisconnectSlot(data->slot_connection);

  auto& ref = data->room.slots_to_be_deleted;
  delete data;
  --ref;

  return GST_PAD_PROBE_REMOVE;
}

void MixerRoom::DisconnectSlot(SlotConnection& slot_connection)
{
  auto& [slot, mixer_sink, tee_src] = slot_connection;

  // State changes to GST_STATE_READY or GST_STATE_NULL never return GST_STATE_CHANGE_ASYNC.
  gst_element_set_state(slot->DecoderBin(), GST_STATE_NULL);

  // unlink and release mixer sink
  gst_pad_unlink(slot->DecoderSrc(), mixer_sink);
  gst_element_release_request_pad(mixer, mixer_sink);
  gst_object_unref(mixer_sink);

  // unlink and release tee src
  gst_pad_unlink(tee_src, slot->EncoderSink());
  gst_element_release_request_pad(tee, tee_src);
  gst_object_unref(tee_src);

  gst_element_set_state(slot->EncoderBin(), GST_STATE_NULL);

  // removing slot from pipeline will drop refcount to zero
  // but we may want to reuse slot in the future,
  // we also still need to free its resources so we increment refcount
  // TODO: check, maybe not needed
  gst_object_ref(slot->DecoderBin());
  gst_object_ref(slot->EncoderBin());
  gst_bin_remove(GST_BIN(pipeline), slot->DecoderBin());
  gst_bin_remove(GST_BIN(pipeline), slot->EncoderBin());
}