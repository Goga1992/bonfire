#ifndef __GST_AUDIO_MIXER_ELEMENTS_H__
#define __GST_AUDIO_MIXER_ELEMENTS_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudioaggregator.h>

#include "gstaudiomixer.h"
// #include "gstaudiointerleave.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL void audiomixer_element_init (GstPlugin * plugin);

GST_ELEMENT_REGISTER_DECLARE (bonaudiomixer);
// GST_ELEMENT_REGISTER_DECLARE (liveadder);
// GST_ELEMENT_REGISTER_DECLARE (audiointerleave);


#endif /* __GST_AUDIO_MIXER_ELEMENTS_H__ */
