#pragma once

#include <gst/gst.h>

#define GST_MIX_INPUT_META_API_TYPE (gst_mix_input_meta_api_get_type())
#define GST_MIX_INPUT_META_INFO (gst_mix_input_meta_get_info())
typedef struct _GstMixInputMeta GstMixInputMeta;

struct SampleFromPad
{
  GstPad* pad = nullptr;
  GstBuffer* inbuf = nullptr;
  guint in_offset;
  guint out_offset;
  guint num_frames;
  gint bpf;
  gint channels;
};

struct _GstMixInputMeta
{
  GstMeta meta;

  SampleFromPad sample;
};

GType gst_mix_input_meta_api_get_type(void);

const GstMetaInfo* gst_mix_input_meta_get_info(void);

#define gst_buffer_get_mix_input_meta(buffer) \
  ((GstMixInputMeta*)gst_buffer_get_meta((buffer), GST_MIX_INPUT_META_API_TYPE))

GstMixInputMeta* gst_buffer_add_mix_input_meta(GstBuffer* buffer, SampleFromPad sample);
