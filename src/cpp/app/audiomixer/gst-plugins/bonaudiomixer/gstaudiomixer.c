#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstaudiomixerelements.h"
#include "gstaudiomixerorc.h"


#define DEFAULT_PAD_VOLUME (1.0)
#define DEFAULT_PAD_MUTE (FALSE)

/* some defines for audio processing */
/* the volume factor is a range from 0.0 to (arbitrary) VOLUME_MAX_DOUBLE = 10.0
 * we map 1.0 to VOLUME_UNITY_INT*
 */
#define VOLUME_UNITY_INT8            8  /* internal int for unity 2^(8-5) */
#define VOLUME_UNITY_INT8_BIT_SHIFT  3  /* number of bits to shift for unity */
#define VOLUME_UNITY_INT16           2048       /* internal int for unity 2^(16-5) */
#define VOLUME_UNITY_INT16_BIT_SHIFT 11 /* number of bits to shift for unity */
#define VOLUME_UNITY_INT24           524288     /* internal int for unity 2^(24-5) */
#define VOLUME_UNITY_INT24_BIT_SHIFT 19 /* number of bits to shift for unity */
#define VOLUME_UNITY_INT32           134217728  /* internal int for unity 2^(32-5) */
#define VOLUME_UNITY_INT32_BIT_SHIFT 27

enum
{
  PROP_PAD_0,
  PROP_PAD_VOLUME,
  PROP_PAD_MUTE,
  PROP_PAD_SAMPLE_CALLBACK_FUNC,
  PROP_PAD_SAMPLE_CALLBACK_USER_DATA,
};

G_DEFINE_TYPE (GstBonAudioMixerPad, gst_bon_audiomixer_pad,
    GST_TYPE_AUDIO_AGGREGATOR_CONVERT_PAD);
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE (bonaudiomixer, "bonaudiomixer",
    GST_RANK_NONE, GST_TYPE_BON_AUDIO_MIXER, audiomixer_element_init (plugin));

static void
gst_bon_audiomixer_pad_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstBonAudioMixerPad *pad = GST_BON_AUDIO_MIXER_PAD (object);

  switch (prop_id) {
    case PROP_PAD_VOLUME:
      g_value_set_double (value, pad->volume);
      break;
    case PROP_PAD_MUTE:
      g_value_set_boolean (value, pad->mute);
      break;
    case PROP_PAD_SAMPLE_CALLBACK_FUNC:
      g_value_set_pointer (value, (gpointer)pad->sample_callback_func);
      break;
    case PROP_PAD_SAMPLE_CALLBACK_USER_DATA:
      g_value_set_pointer (value, pad->sample_callback_user_data);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_bon_audiomixer_pad_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstBonAudioMixerPad *pad = GST_BON_AUDIO_MIXER_PAD (object);

  switch (prop_id) {
    case PROP_PAD_VOLUME:
      GST_OBJECT_LOCK (pad);
      pad->volume = g_value_get_double (value);
      pad->volume_i8 = pad->volume * VOLUME_UNITY_INT8;
      pad->volume_i16 = pad->volume * VOLUME_UNITY_INT16;
      pad->volume_i32 = pad->volume * VOLUME_UNITY_INT32;
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_MUTE:
      GST_OBJECT_LOCK (pad);
      pad->mute = g_value_get_boolean (value);
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_SAMPLE_CALLBACK_FUNC:
      GST_OBJECT_LOCK (pad);
      pad->sample_callback_func = (GstSampleCallbackFunc) g_value_get_pointer (value);
      GST_OBJECT_UNLOCK (pad);
      break;
    case PROP_PAD_SAMPLE_CALLBACK_USER_DATA:
      GST_OBJECT_LOCK (pad);
      pad->sample_callback_user_data = g_value_get_pointer (value);
      GST_OBJECT_UNLOCK (pad);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_bon_audiomixer_pad_class_init (GstBonAudioMixerPadClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;

  gobject_class->set_property = gst_bon_audiomixer_pad_set_property;
  gobject_class->get_property = gst_bon_audiomixer_pad_get_property;

  g_object_class_install_property (gobject_class, PROP_PAD_VOLUME,
      g_param_spec_double ("volume", "Volume", "Volume of this pad",
          0.0, 10.0, DEFAULT_PAD_VOLUME,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PAD_MUTE,
      g_param_spec_boolean ("mute", "Mute", "Mute this pad",
          DEFAULT_PAD_MUTE,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  // BON properties
  g_object_class_install_property (gobject_class, PROP_PAD_SAMPLE_CALLBACK_FUNC,
      g_param_spec_pointer ("sample-callback-func", "Sample callback function", "Will be called during aggregate_one_buffer",
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PAD_SAMPLE_CALLBACK_USER_DATA,
      g_param_spec_pointer ("sample-callback-user-data", "Sample callback user_data", "user_data for sample-callback-func",
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
}

static void
gst_bon_audiomixer_pad_init (GstBonAudioMixerPad * pad)
{
  pad->volume = DEFAULT_PAD_VOLUME;
  pad->mute = DEFAULT_PAD_MUTE;
}

enum
{
  PROP_0
};

/* These are the formats we can mix natively */

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define CAPS \
  GST_AUDIO_CAPS_MAKE ("{ S32LE, U32LE, S16LE, U16LE, S8, U8, F32LE, F64LE }") \
  ", layout = interleaved"
#else
#define CAPS \
  GST_AUDIO_CAPS_MAKE ("{ S32BE, U32BE, S16BE, U16BE, S8, U8, F32BE, F64BE }") \
  ", layout = interleaved"
#endif

static GstStaticPadTemplate gst_bon_audiomixer_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS)
    );

#define SINK_CAPS \
  GST_STATIC_CAPS (GST_AUDIO_CAPS_MAKE (GST_AUDIO_FORMATS_ALL) \
      ", layout=interleaved")

static GstStaticPadTemplate gst_bon_audiomixer_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    SINK_CAPS);

static void gst_bon_audiomixer_child_proxy_init (gpointer g_iface,
    gpointer iface_data);

#define gst_bon_audiomixer_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstBonAudioMixer, gst_bon_audiomixer,
    GST_TYPE_AUDIO_AGGREGATOR, G_IMPLEMENT_INTERFACE (GST_TYPE_CHILD_PROXY,
        gst_bon_audiomixer_child_proxy_init));

static GstPad *gst_bon_audiomixer_request_new_pad (GstElement * element,
    GstPadTemplate * temp, const gchar * req_name, const GstCaps * caps);
static void gst_bon_audiomixer_release_pad (GstElement * element, GstPad * pad);

static gboolean
gst_bon_audiomixer_aggregate_one_buffer (GstAudioAggregator * aagg,
    GstAudioAggregatorPad * aaggpad, GstBuffer * inbuf, guint in_offset,
    GstBuffer * outbuf, guint out_offset, guint num_samples);


static void
gst_bon_audiomixer_class_init (GstBonAudioMixerClass * klass)
{
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstAudioAggregatorClass *aagg_class = (GstAudioAggregatorClass *) klass;

  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &gst_bon_audiomixer_src_template, GST_TYPE_AUDIO_AGGREGATOR_CONVERT_PAD);
  gst_element_class_add_static_pad_template_with_gtype (gstelement_class,
      &gst_bon_audiomixer_sink_template, GST_TYPE_BON_AUDIO_MIXER_PAD);
  gst_element_class_set_static_metadata (gstelement_class, "BonAudioMixer",
      "Generic/Audio", "Mixes multiple audio streams",
      "Sebastian Dröge <sebastian@centricular.com>");

  gstelement_class->request_new_pad =
      GST_DEBUG_FUNCPTR (gst_bon_audiomixer_request_new_pad);
  gstelement_class->release_pad =
      GST_DEBUG_FUNCPTR (gst_bon_audiomixer_release_pad);

  aagg_class->aggregate_one_buffer = gst_bon_audiomixer_aggregate_one_buffer;

  gst_type_mark_as_plugin_api (GST_TYPE_BON_AUDIO_MIXER_PAD, 0);
}

static void
gst_bon_audiomixer_init (GstBonAudioMixer * audiomixer)
{
}

static void default_sample_callback(gpointer user_data,
                                    GstPad * pad,
                                    GstBuffer * inbuf,
                                    GstBuffer * outbuf,
                                    guint in_offset,
                                    guint out_offset,
                                    guint num_frames,
                                    gint bpf,
                                    gint channels)
{
}

static GstPad *
gst_bon_audiomixer_request_new_pad (GstElement * element, GstPadTemplate * templ,
    const gchar * req_name, const GstCaps * caps)
{
  GstBonAudioMixerPad *newpad;

  newpad = (GstBonAudioMixerPad *)
      GST_ELEMENT_CLASS (parent_class)->request_new_pad (element,
      templ, req_name, caps);

  if (newpad == NULL)
    goto could_not_create;

  newpad->sample_callback_func = (GstSampleCallbackFunc) default_sample_callback;
  newpad->sample_callback_user_data = NULL;

  gst_child_proxy_child_added (GST_CHILD_PROXY (element), G_OBJECT (newpad),
      GST_OBJECT_NAME (newpad));

  return GST_PAD_CAST (newpad);

could_not_create:
  {
    GST_DEBUG_OBJECT (element, "could not create/add  pad");
    return NULL;
  }
}

static void
gst_bon_audiomixer_release_pad (GstElement * element, GstPad * pad)
{
  GstBonAudioMixer *audiomixer;

  audiomixer = GST_BON_AUDIO_MIXER (element);

  GST_DEBUG_OBJECT (audiomixer, "release pad %s:%s", GST_DEBUG_PAD_NAME (pad));

  gst_child_proxy_child_removed (GST_CHILD_PROXY (audiomixer), G_OBJECT (pad),
      GST_OBJECT_NAME (pad));

  GST_ELEMENT_CLASS (parent_class)->release_pad (element, pad);
}

static gboolean
gst_bon_audiomixer_aggregate_one_buffer (GstAudioAggregator * aagg,
    GstAudioAggregatorPad * aaggpad, GstBuffer * inbuf, guint in_offset,
    GstBuffer * outbuf, guint out_offset, guint num_frames)
{
  GstBonAudioMixerPad *pad = GST_BON_AUDIO_MIXER_PAD (aaggpad);
  GstMapInfo inmap;
  GstMapInfo outmap;
  gint bpf;
  GstAggregator *agg = GST_AGGREGATOR (aagg);
  GstAudioAggregatorPad *srcpad = GST_AUDIO_AGGREGATOR_PAD (agg->srcpad);

  GST_OBJECT_LOCK (aagg);
  GST_OBJECT_LOCK (aaggpad);

  if (pad->mute || pad->volume < G_MINDOUBLE) {
    GST_DEBUG_OBJECT (pad, "Skipping muted pad");
    GST_OBJECT_UNLOCK (aaggpad);
    GST_OBJECT_UNLOCK (aagg);
    return FALSE;
  }

  bpf = GST_AUDIO_INFO_BPF (&srcpad->info);

  pad->sample_callback_func(pad->sample_callback_user_data,
                            GST_PAD_CAST(pad),
                            inbuf,
                            outbuf,
                            in_offset,
                            out_offset,
                            num_frames,
                            bpf,
                            srcpad->info.channels);

  gst_buffer_map (outbuf, &outmap, GST_MAP_READWRITE);
  gst_buffer_map (inbuf, &inmap, GST_MAP_READ);
  GST_LOG_OBJECT (pad, "mixing %u bytes at offset %u from offset %u",
      num_frames * bpf, out_offset * bpf, in_offset * bpf);

  /* further buffers, need to add them */
  if (pad->volume == 1.0) {
    switch (srcpad->info.finfo->format) {
      case GST_AUDIO_FORMAT_U8:
        audiomixer_orc_add_u8 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S8:
        audiomixer_orc_add_s8 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_U16:
        audiomixer_orc_add_u16 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S16:
        audiomixer_orc_add_s16 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_U32:
        audiomixer_orc_add_u32 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S32:
        audiomixer_orc_add_s32 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_F32:
        audiomixer_orc_add_f32 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_F64:
        audiomixer_orc_add_f64 ((gpointer) (outmap.data + out_offset * bpf),
            (gpointer) (inmap.data + in_offset * bpf),
            num_frames * srcpad->info.channels);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  } else {
    switch (srcpad->info.finfo->format) {
      case GST_AUDIO_FORMAT_U8:
        audiomixer_orc_add_volume_u8 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i8, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S8:
        audiomixer_orc_add_volume_s8 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i8, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_U16:
        audiomixer_orc_add_volume_u16 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i16, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S16:
        audiomixer_orc_add_volume_s16 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i16, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_U32:
        audiomixer_orc_add_volume_u32 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i32, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_S32:
        audiomixer_orc_add_volume_s32 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume_i32, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_F32:
        audiomixer_orc_add_volume_f32 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume, num_frames * srcpad->info.channels);
        break;
      case GST_AUDIO_FORMAT_F64:
        audiomixer_orc_add_volume_f64 ((gpointer) (outmap.data +
                out_offset * bpf), (gpointer) (inmap.data + in_offset * bpf),
            pad->volume, num_frames * srcpad->info.channels);
        break;
      default:
        g_assert_not_reached ();
        break;
    }
  }
  gst_buffer_unmap (inbuf, &inmap);
  gst_buffer_unmap (outbuf, &outmap);

  GST_OBJECT_UNLOCK (aaggpad);
  GST_OBJECT_UNLOCK (aagg);

  return TRUE;
}


/* GstChildProxy implementation */
static GObject *
gst_bon_audiomixer_child_proxy_get_child_by_index (GstChildProxy * child_proxy,
    guint index)
{
  GstBonAudioMixer *audiomixer = GST_BON_AUDIO_MIXER (child_proxy);
  GObject *obj = NULL;

  GST_OBJECT_LOCK (audiomixer);
  obj = g_list_nth_data (GST_ELEMENT_CAST (audiomixer)->sinkpads, index);
  if (obj)
    gst_object_ref (obj);
  GST_OBJECT_UNLOCK (audiomixer);

  return obj;
}

static guint
gst_bon_audiomixer_child_proxy_get_children_count (GstChildProxy * child_proxy)
{
  guint count = 0;
  GstBonAudioMixer *audiomixer = GST_BON_AUDIO_MIXER (child_proxy);

  GST_OBJECT_LOCK (audiomixer);
  count = GST_ELEMENT_CAST (audiomixer)->numsinkpads;
  GST_OBJECT_UNLOCK (audiomixer);
  GST_INFO_OBJECT (audiomixer, "Children Count: %d", count);

  return count;
}

static void
gst_bon_audiomixer_child_proxy_init (gpointer g_iface, gpointer iface_data)
{
  GstChildProxyInterface *iface = g_iface;

  GST_INFO ("initializing child proxy interface");
  iface->get_child_by_index = gst_bon_audiomixer_child_proxy_get_child_by_index;
  iface->get_children_count = gst_bon_audiomixer_child_proxy_get_children_count;
}
