#include "videoscaler_slot.hpp"

#include "logger/logger.hpp"
#include "utils/utils.hpp"

#include <fmt/format.h>

#include <random>

VideoScalerSlot::VideoScalerSlot(const SlotId slot_id,
                                 const SlotConfig& cfg)
: id(slot_id),
  pipeline(gst_pipeline_new(slot_id.c_str()))
{
  InitDecoder();

  size_t sink_port_idx = 0;
  for (const auto& [branch_name, w, h] : RESOLUTIONS)
  {
    InitBranch(branch_name, cfg.sink_hostname, cfg.sink_ports[sink_port_idx++]);
  }

  gst_debug_bin_to_dot_file_with_ts(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, id.c_str());

  auto ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
  bon::utils::Assert(ret == GST_STATE_CHANGE_SUCCESS, "State change should be GST_STATE_CHANGE_SUCCESS");

  g_object_get(udpsrc, "port", &udpsrc_port, NULL);
}

GstElement* VideoScalerSlot::MakeElement(std::string type, std::optional<std::string> name)
{
  auto element_id = name.value_or(type) + "_" + id;
  GstElement* element = gst_element_factory_make(type.c_str(), element_id.c_str());
  bon::utils::Assert(element != NULL, "Element should not be NULL");

  // transfers ownership to bin
  gst_bin_add(GST_BIN(pipeline), element);

  return element;
}

void VideoScalerSlot::InitDecoder()
{
  udpsrc = MakeElement("udpsrc");
  auto rtpjitterbuffer = MakeElement("rtpjitterbuffer");
  auto rtph264depay = MakeElement("rtph264depay");
  auto h264parse = MakeElement("h264parse");
  auto avdec_h264 = MakeElement("avdec_h264");
  tee = MakeElement("tee");

  gst_element_link_many(udpsrc,
                        rtpjitterbuffer,
                        rtph264depay,
                        h264parse,
                        avdec_h264,
                        tee,
                        NULL);

  g_object_set(udpsrc, "port", 0, "do-timestamp", true, NULL);
  g_object_set(rtpjitterbuffer, "do-lost", true, "do-timestamp", true, NULL);

  GstCaps* capsfilter_udpsrc = gst_caps_new_simple("application/x-rtp",
                                                   "clock-rate", G_TYPE_INT, 90000,
                                                   "payload", G_TYPE_INT, 96,
                                                   "encoding-name", G_TYPE_STRING, "H264",
                                                   NULL);
  g_object_set(udpsrc, "caps", capsfilter_udpsrc, NULL);
  gst_caps_unref(capsfilter_udpsrc);

  auto avdec_h264_sink = gst_element_get_static_pad(avdec_h264, "sink");
  gst_pad_add_probe(avdec_h264_sink, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, CapsProbe, &caps_probe_data, NULL);
  gst_object_unref(avdec_h264_sink);
}

GstPadProbeReturn VideoScalerSlot::CapsProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
  auto event = gst_pad_probe_info_get_event(info);
  if (GST_EVENT_TYPE(event) != GST_EVENT_CAPS)
  {
    return GST_PAD_PROBE_OK;
  }

  GstCaps *caps;
  gst_event_parse_caps(event, &caps);

  auto structure = gst_caps_get_structure(caps, 0);
  gchar* structure_text = gst_structure_to_string(structure);

  VideoCaps in_caps;
  gst_structure_get_int(structure, "width", &in_caps.width);
  gst_structure_get_int(structure, "height", &in_caps.height);

  auto data = static_cast<CapsProbeData*>(user_data);
  if (data->last_caps == in_caps)
  {
    return GST_PAD_PROBE_OK;
  }

  auto branch_idx = MatchCapsToBranch(in_caps);

  bon::log::Info("Incoming caps update: matched_branch=[{}], width=[{}], height=[{}]",
                 RESOLUTIONS[branch_idx].name, in_caps.width, in_caps.height);

  for (size_t i = 0; i < std::size(RESOLUTIONS); ++i)
  {
    auto [width_downgrade, height_downgrade] = i > branch_idx
                                               ? GetDowngradeFactor(branch_idx, i)
                                               : std::pair<float, float>{1, 1};
    if (in_caps.width < in_caps.height)
    {
      std::swap(width_downgrade, height_downgrade);
    }

    int width_downgraded = in_caps.width / width_downgrade;
    int height_downgraded = in_caps.height / height_downgrade;

    width_downgraded -= width_downgraded % 2;
    height_downgraded -= height_downgraded % 2;

    auto capsfilter = data->branch_caps_filters[i].second;
    GstCaps* branch_caps = gst_caps_new_simple("video/x-raw",
                                               "width", G_TYPE_INT, width_downgraded,
                                               "height", G_TYPE_INT, height_downgraded,
                                               NULL);
    g_object_set(capsfilter, "caps", branch_caps, NULL);
    gst_caps_unref(branch_caps);
  }

  data->last_caps = in_caps;

  return GST_PAD_PROBE_OK;
}

std::pair<float, float> VideoScalerSlot::GetDowngradeFactor(size_t res_idx_0, size_t res_idx_1)
{
  const auto& res0 = RESOLUTIONS[res_idx_0];
  const auto& res1 = RESOLUTIONS[res_idx_1];
  return {float(res0.width) / float(res1.width), float(res0.height) / float(res1.height)};
}

size_t VideoScalerSlot::MatchCapsToBranch(const VideoCaps& caps)
{
  int64_t caps_resolution = caps.width * caps.height;

  size_t min_diff = std::numeric_limits<size_t>::max();
  size_t idx = 0;

  for (size_t i = 0; i < std::size(RESOLUTIONS); ++i)
  {
    const auto& [_, width, height] = RESOLUTIONS[i];
    auto diff = std::abs(caps_resolution - width * height);
    if (diff < min_diff)
    {
      min_diff = diff;
      idx = i;
    }
  }

  return idx;
}

void VideoScalerSlot::InitBranch(std::string_view branch_name, const std::string& sink_hostname, uint32_t sink_port)
{
    const auto make_element_branch = [&](std::string type, std::optional<std::string> elname = std::nullopt)
    {
      return MakeElement(type, elname.value_or(type) + "_" + std::string(branch_name));
    };

    auto queue = make_element_branch("queue");
    auto valve = make_element_branch("valve");
    auto videoconvert = make_element_branch("videoconvert");
    auto videoscale = make_element_branch("videoscale");
    auto capsfilter = make_element_branch("capsfilter");
    auto queue_enc = make_element_branch("queue", "queue_enc");
    auto x264enc = make_element_branch("x264enc");
    auto rtph264pay = make_element_branch("rtph264pay");
    auto udpsink = make_element_branch("udpsink");

    gst_element_link_many(queue,
                          valve,
                          videoscale,
                          capsfilter,
                          queue_enc,
                          x264enc,
                          rtph264pay,
                          udpsink,
                          NULL);

    GstPad* tee_src = gst_element_request_pad_simple(tee, "src_%u");
    GstPad* queue_sink = gst_element_get_static_pad(queue, "sink");
    gst_pad_link(tee_src, queue_sink);
    gst_object_unref(queue_sink);
    tee_srcs.push_back(tee_src);

    g_object_set(valve, "drop", true, NULL);

    g_object_set(x264enc, "tune", 4, "speed-preset", 3, "bframes", 0, "key-int-max", 0, NULL);

    g_object_set(udpsink,
                 "host", sink_hostname.c_str(),
                 "port", sink_port,
                 "async", false,
                 "sync", false,
                 NULL);

    branch_valves.emplace_back(branch_name, valve);
    branch_encoders.emplace_back(branch_name, x264enc);
    caps_probe_data.branch_caps_filters.emplace_back(branch_name, capsfilter);
}

void VideoScalerSlot::SetBranchActive(const std::string& name, bool active)
{
  auto it = std::find_if(branch_valves.begin(), branch_valves.end(),
                         [&](const auto& p) { return p.first == name; });

  if (it == branch_valves.end())
  {
    bon::log::Error("Could not find branch: name=[{}]", name);
    throw std::runtime_error("could not find branch");
  }

  bool drop_active = not active;

  // check if branch is already in the same state
  gboolean cur_drop_active;
  g_object_get(it->second, "drop", &cur_drop_active, NULL);
  if (cur_drop_active == drop_active)
  {
    return;
  }

  // update only if desired state not equals current
  g_object_set(it->second, "drop", drop_active, NULL);
  num_active_branches += (drop_active ? -1 : 1);
  bon::log::Info("Num active branches: [{}]", num_active_branches.load());
}

void VideoScalerSlot::ForceKeyFrame(const std::string& name)
{
  auto it = std::find_if(branch_encoders.begin(), branch_encoders.end(),
                         [&](const auto& p) { return p.first == name; });

  if (it == branch_encoders.end())
  {
    bon::log::Error("Could not find branch: name=[{}]", name);
    throw std::runtime_error("could not find branch");
  }

  auto enc_src = gst_element_get_static_pad(it->second, "src");
  gst_pad_send_event(enc_src, gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM,
                                                   gst_structure_new("GstForceKeyUnit", "all-headers", G_TYPE_BOOLEAN, TRUE, NULL)));
  gst_object_unref(enc_src);
}

VideoScalerSlot::~VideoScalerSlot()
{
  auto ret = gst_element_set_state(pipeline, GST_STATE_NULL);
  bon::utils::Assert(ret == GST_STATE_CHANGE_SUCCESS, "State change should be GST_STATE_CHANGE_SUCCESS");

  for (const auto tee_src : tee_srcs)
  {
    gst_element_release_request_pad(tee, tee_src);
    gst_object_unref(tee_src);
  }

  gst_object_unref(pipeline);
}