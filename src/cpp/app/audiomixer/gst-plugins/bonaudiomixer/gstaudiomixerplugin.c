#include "gstaudiomixerelements.h"

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;

  ret |= GST_ELEMENT_REGISTER (bonaudiomixer, plugin);
  // ret |= GST_ELEMENT_REGISTER (liveadder, plugin);
  // ret |= GST_ELEMENT_REGISTER (audiointerleave, plugin);

  return ret;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    bonaudiomixer,
    "Mixes multiple audio streams",
    plugin_init, "0.0.1", "LGPL", "GStreamer template Plug-ins", "https://gstreamer.freedesktop.org")
