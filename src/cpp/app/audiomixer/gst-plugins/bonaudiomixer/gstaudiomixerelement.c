#include "gstaudiomixerelements.h"

#define GST_CAT_DEFAULT gst_audiomixer_debug
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);

void
audiomixer_element_init (GstPlugin * plugin)
{
  static gsize res = FALSE;
  if (g_once_init_enter (&res)) {
    GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, "audiomixer", 0,
        "audio mixing element");
    g_once_init_leave (&res, TRUE);
  }
}
