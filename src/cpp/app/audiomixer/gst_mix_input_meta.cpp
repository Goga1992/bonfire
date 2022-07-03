#include "gst_mix_input_meta.hpp"

#include <logger/logger.hpp>

static gboolean gst_mix_input_meta_init(GstMeta* meta, gpointer params, GstBuffer* buffer)
{
  return TRUE;
}

static void gst_mix_input_meta_free(GstMeta* meta, GstBuffer* buffer)
{
  GstMixInputMeta* mix_meta = (GstMixInputMeta*)meta;
  gst_buffer_unref(mix_meta->sample.inbuf);
}

static gboolean gst_mix_input_meta_transform(GstBuffer* dest,
                                             GstMeta* meta,
                                             GstBuffer* buffer,
                                             GQuark type,
                                             gpointer data)
{
  GstMixInputMeta* src = (GstMixInputMeta*)meta;
  GstMixInputMeta* dst = (GstMixInputMeta*)gst_buffer_add_meta(dest, GST_MIX_INPUT_META_INFO, NULL);

  gst_buffer_ref(src->sample.inbuf);
  dst->sample = src->sample;

  return TRUE;
}

GType gst_mix_input_meta_api_get_type(void)
{
  static volatile GType type = 0;
  static const gchar* tags[] = {};

  if (g_once_init_enter(&type))
  {
    GType _type = gst_meta_api_type_register("GstMixInputMetaAPI", tags);
    g_once_init_leave(&type, _type);
  }

  return type;
}

/* mix_input metadata*/
const GstMetaInfo* gst_mix_input_meta_get_info(void)
{
  static const GstMetaInfo* mix_input_meta_info = NULL;

  if (g_once_init_enter((GstMetaInfo**)&mix_input_meta_info))
  {
    const GstMetaInfo* meta = gst_meta_register(GST_MIX_INPUT_META_API_TYPE,
                                                "GstMixInputMeta",
                                                sizeof(GstMixInputMeta),
                                                (GstMetaInitFunction)gst_mix_input_meta_init,
                                                (GstMetaFreeFunction)gst_mix_input_meta_free,
                                                (GstMetaTransformFunction)gst_mix_input_meta_transform);

    g_once_init_leave((GstMetaInfo**)&mix_input_meta_info, (GstMetaInfo*)meta);
  }

  return mix_input_meta_info;
}

GstMixInputMeta* gst_buffer_add_mix_input_meta(GstBuffer* buffer, SampleFromPad sample)
{
  GstMixInputMeta* meta;

  meta = (GstMixInputMeta*)gst_buffer_add_meta(buffer, GST_MIX_INPUT_META_INFO, NULL);

  if (!meta)
  {
    return NULL;
  }

  gst_buffer_ref(sample.inbuf);
  meta->sample = sample;

  return meta;
}