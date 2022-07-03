#pragma once

#include "audiomixer_slot.hpp"

#include <gst/gst.h>

#include <atomic>
#include <memory>
#include <vector>

class MixerRoom
{
 public:
  MixerRoom(const RoomId room_id, const size_t num_speakers);

  MixerRoom(const MixerRoom&) = delete;
  MixerRoom& operator=(const MixerRoom&) = delete;

  ~MixerRoom();

 public:
  std::optional<uint32_t> AddSlot(const SlotId slot_id, const SlotConfig& cfg);
  void RemoveSlot(const SlotId& slot_id);

  size_t Size() const { return slots.size(); }
  size_t SlotsToBeDeleted() const { return slots_to_be_deleted; }

 private:
  struct SlotConnection
  {
    std::unique_ptr<MixerSlot> slot;

    GstPad* mixer_sink; // store sink so it can be released later
    GstPad* tee_src; // same
  };

  struct DisconnectSlotProbeData
  {
    SlotConnection slot_connection;
    MixerRoom& room;
  };

 private:
  static void SampleCallbackFunc(gpointer user_data,
                                 GstPad* pad,
                                 GstBuffer* inbuf,
                                 GstBuffer* outbuf,
                                 guint in_offset,
                                 guint out_offset,
                                 guint num_frames,
                                 gint bpf,
                                 gint channels);

  static GstPadProbeReturn MixMinusProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);

  static GstPadProbeReturn DisconnectSlotWhenIdleProbe(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
  void DisconnectSlot(SlotConnection& slot_connection);

 private:
  RoomId id;

  GstElement* pipeline = nullptr;
  GstElement* mixer = nullptr;
  GstElement* tee = nullptr;

  std::vector<SlotConnection> slots;
  std::atomic<size_t> slots_to_be_deleted = 0;
};