#ifndef __GST_AUDIO_MIXER_H__
#define __GST_AUDIO_MIXER_H__

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudioaggregator.h>

G_BEGIN_DECLS

#define GST_TYPE_BON_AUDIO_MIXER (gst_bon_audiomixer_get_type())
G_DECLARE_FINAL_TYPE (GstBonAudioMixer, gst_bon_audiomixer, GST, BON_AUDIO_MIXER,
    GstAudioAggregator)

/**
 * GstAudioMixer:
 *
 * The audiomixer object structure.
 */
struct _GstBonAudioMixer {
  GstAudioAggregator element;
};

#define GST_TYPE_BON_AUDIO_MIXER_PAD (gst_bon_audiomixer_pad_get_type())
G_DECLARE_FINAL_TYPE (GstBonAudioMixerPad, gst_bon_audiomixer_pad,
    GST, BON_AUDIO_MIXER_PAD, GstAudioAggregatorConvertPad)

typedef void (*GstSampleCallbackFunc) (gpointer user_data,
                                       GstPad * pad,
                                       GstBuffer* inbuf,
                                       GstBuffer* outbuf,
                                       guint in_offset,
                                       guint out_offset,
                                       guint num_frames,
                                       gint bpf,
                                       gint channels);

struct _GstBonAudioMixerPad {
  GstAudioAggregatorConvertPad parent;

  gdouble volume;
  gint volume_i32;
  gint volume_i16;
  gint volume_i8;
  gboolean mute;

  GstSampleCallbackFunc sample_callback_func;
  gpointer sample_callback_user_data;
};

G_END_DECLS

#endif /* __GST_AUDIO_MIXER_H__ */
