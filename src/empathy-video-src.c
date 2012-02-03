/*
 * empathy-gst-video-src.c - Source for EmpathyGstVideoSrc
 * Copyright (C) 2008 Collabora Ltd.
 * @author Sjoerd Simons <sjoerd.simons@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */


#include <stdio.h>
#include <stdlib.h>

#include <gst/interfaces/colorbalance.h>

#define DEBUG_FLAG EMPATHY_DEBUG_VOIP
#include <libempathy/empathy-debug.h>

#include "empathy-video-src.h"

G_DEFINE_TYPE(EmpathyGstVideoSrc, empathy_video_src, GST_TYPE_BIN)

/* Keep in sync with EmpathyGstVideoSrcChannel */
static const gchar *channel_names[NR_EMPATHY_GST_VIDEO_SRC_CHANNELS] = {
  "contrast", "brightness", "gamma" };

/* signal enum */
#if 0
enum
{
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = {0};
#endif

/* private structure */
typedef struct _EmpathyGstVideoSrcPrivate EmpathyGstVideoSrcPrivate;

struct _EmpathyGstVideoSrcPrivate
{
  gboolean dispose_has_run;
  GstElement *src;
  /* Element implementing a ColorBalance interface */
  GstElement *balance;
  /* Elements for resolution and framerate adjustment */
  GstElement *capsfilter;
  GstElement *videorate;
  guint width;
  guint height;
  guint framerate;
};

#define EMPATHY_GST_VIDEO_SRC_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EMPATHY_TYPE_GST_VIDEO_SRC, \
    EmpathyGstVideoSrcPrivate))
/**
 * empathy_gst_add_to_bin - create a new gst element, add to bin and link it.
 * @bin - bin to add the new element to.
 * @src - src element for the new element (may be NULL).
 * @name - name of the factory for the new element
 *
 * Returns: The newly created element ot %NULL on failure
 */
static GstElement *
empathy_gst_add_to_bin (GstBin *bin,
  GstElement *src,
  const gchar *factoryname)
{
  GstElement *ret;

  if ((ret = gst_element_factory_make (factoryname, NULL)) == NULL)
  {
    g_message ("Element factory \"%s\" not found.", factoryname);
    goto error;
  }

  if (!gst_bin_add (bin, ret))
  {
    g_warning ("Couldn't add \"%s\" to bin.", factoryname);
    goto error;
  }

  /* do not link if src == NULL, just exit here */
  if (src == NULL)
    return ret;

  if (!gst_element_link (src, ret))
  {
    g_warning ("Failed to link \"%s\".", factoryname);
    gst_bin_remove (bin, ret);
    goto error;
  }

  return ret;

error:
  if (ret != NULL)
    gst_object_unref (ret);
  return NULL;
}

static gboolean
empathy_video_src_drop_eos (GstPad *pad, GstEvent *event, gpointer user_data)
{
  return GST_EVENT_TYPE (event) != GST_EVENT_EOS;
}

static void
empathy_video_src_init (EmpathyGstVideoSrc *obj)
{
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (obj);
  GstElement *element, *element_back;
  GstPad *ghost, *src;
  GstCaps *caps;
  gchar *str;

  /* allocate caps here, so we can update it by optional elements */
  caps = gst_caps_new_simple ("video/x-raw-yuv",
    "width", G_TYPE_INT, 320,
    "height", G_TYPE_INT, 240,
    NULL);

  /* allocate any data required by the object here */
  if ((element = empathy_gst_add_to_bin (GST_BIN (obj),
      NULL, "v4l2src")) == NULL)
    g_error ("Couldn't add \"v4l2src\" (gst-plugins-good missing?)");

  /* we need to save our source to priv->src */
  priv->src = element;

  /* Drop EOS events, so that our sinks don't get confused when we restart the
   * source (triggering an EOS) */
  src = gst_element_get_static_pad (element, "src");
  gst_pad_add_event_probe (src, G_CALLBACK (empathy_video_src_drop_eos), NULL);
  gst_object_unref (src);

  /* videorate with the required properties optional as it needs a currently
   * unreleased gst-plugins-base 0.10.36 */
  element_back = element;
  element = empathy_gst_add_to_bin (GST_BIN (obj), element, "videorate");

  if (element != NULL && g_object_class_find_property (
      G_OBJECT_GET_CLASS (element), "max-rate") != NULL)
    {
      priv->videorate = element;
      g_object_set (G_OBJECT (element),
        "drop-only", TRUE,
        "average-period", GST_SECOND/2,
        NULL);
    }
  else
    {
      g_message ("videorate missing or doesn't have max-rate property, not"
        "doing dynamic framerate changes (Needs gst-plugins-base >= 0.10.36)");
      /* Refcount owned by the bin */
      gst_bin_remove (GST_BIN (obj), element);
      element = element_back;
    }

  gst_caps_set_simple (caps,
      "framerate", GST_TYPE_FRACTION_RANGE, 1, 1, 30, 1,
      NULL);

  str = gst_caps_to_string (caps);
  DEBUG ("Current video src caps are : %s", str);
  g_free (str);

  if ((element = empathy_gst_add_to_bin (GST_BIN (obj),
      element, "ffmpegcolorspace")) == NULL)
    g_error ("Failed to add \"ffmpegcolorspace\" (gst-plugins-base missing?)");

  if ((element = empathy_gst_add_to_bin (GST_BIN (obj),
      element, "videoscale")) == NULL)
    g_error ("Failed to add \"videoscale\", (gst-plugins-base missing?)");

  if ((element = empathy_gst_add_to_bin (GST_BIN (obj),
      element, "capsfilter")) == NULL)
    g_error (
      "Failed to add \"capsfilter\" (gstreamer core elements missing?)");

  priv->capsfilter = element;
  g_object_set (G_OBJECT (element), "caps", caps, NULL);


  /* optionally add postproc_tmpnoise to improve the performance of encoders */
  element_back = element;
  if ((element = empathy_gst_add_to_bin (GST_BIN (obj),
      element, "postproc_tmpnoise")) == NULL)
    {
      g_message ("Failed to add \"postproc_tmpnoise\" (gst-ffmpeg missing?)");
      element = element_back;
    }

  src = gst_element_get_static_pad (element, "src");
  g_assert (src != NULL);

  ghost = gst_ghost_pad_new ("src", src);
  if (ghost == NULL)
    g_error ("Unable to create ghost pad for the videosrc");

  if (!gst_element_add_pad (GST_ELEMENT (obj), ghost))
    g_error ("pad with the same name already existed or "
            "the pad already had another parent.");

  gst_object_unref (G_OBJECT (src));
}

static void empathy_video_src_dispose (GObject *object);
static void empathy_video_src_finalize (GObject *object);

static void
empathy_video_src_class_init (EmpathyGstVideoSrcClass *empathy_video_src_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (empathy_video_src_class);

  g_type_class_add_private (empathy_video_src_class,
    sizeof (EmpathyGstVideoSrcPrivate));

  object_class->dispose = empathy_video_src_dispose;
  object_class->finalize = empathy_video_src_finalize;
}

void
empathy_video_src_dispose (GObject *object)
{
  EmpathyGstVideoSrc *self = EMPATHY_GST_VIDEO_SRC (object);
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  /* release any references held by the object here */

  if (G_OBJECT_CLASS (empathy_video_src_parent_class)->dispose)
    G_OBJECT_CLASS (empathy_video_src_parent_class)->dispose (object);
}

void
empathy_video_src_finalize (GObject *object)
{
  //EmpathyGstVideoSrc *self = EMPATHY_GST_VIDEO_SRC (object);
  //EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (self);

  /* free any data held directly by the object here */

  G_OBJECT_CLASS (empathy_video_src_parent_class)->finalize (object);
}

GstElement *
empathy_video_src_new (void)
{
  static gboolean registered = FALSE;

  if (!registered) {
    if (!gst_element_register (NULL, "empathyvideosrc",
            GST_RANK_NONE, EMPATHY_TYPE_GST_VIDEO_SRC))
      return NULL;
    registered = TRUE;
  }
  return gst_element_factory_make ("empathyvideosrc", NULL);
}

static GstColorBalance *
dup_color_balance (GstElement *src)
{
  GstElement *color;

  /* Find something supporting GstColorBalance */
  color = gst_bin_get_by_interface (GST_BIN (src), GST_TYPE_COLOR_BALANCE);

  if (color == NULL)
    return NULL;

  /* colorbalance is wrapped by GstImplementsInterface, we
   * need to check if it is actually supported for this instance
   * in its current state before trying to use it */
  if (!GST_IS_COLOR_BALANCE (color))
    {
      g_object_unref (color);
      return NULL;
    }

  return GST_COLOR_BALANCE (color);
}

void
empathy_video_src_set_channel (GstElement *src,
  EmpathyGstVideoSrcChannel channel, guint percent)
{
  GstColorBalance *balance;
  const GList *channels;
  GList *l;

  balance = dup_color_balance (src);
  if (balance == NULL)
    return;

  channels = gst_color_balance_list_channels (balance);

  for (l = (GList *) channels; l != NULL; l = g_list_next (l))
    {
      GstColorBalanceChannel *c = GST_COLOR_BALANCE_CHANNEL (l->data);

      if (g_ascii_strcasecmp (c->label, channel_names[channel]) == 0)
        {
          gst_color_balance_set_value (balance, c,
            ((c->max_value - c->min_value) * percent)/100
              + c->min_value);
          break;
        }
    }

  g_object_unref (balance);
}

guint
empathy_video_src_get_channel (GstElement *src,
  EmpathyGstVideoSrcChannel channel)
{
  GstColorBalance *balance;
  const GList *channels;
  GList *l;
  guint percent = 0;

  balance = dup_color_balance (src);
  if (balance == NULL)
    return percent;

  channels = gst_color_balance_list_channels (balance);

  for (l = (GList *) channels; l != NULL; l = g_list_next (l))
    {
      GstColorBalanceChannel *c = GST_COLOR_BALANCE_CHANNEL (l->data);

      if (g_ascii_strcasecmp (c->label, channel_names[channel]) == 0)
        {
          percent =
            ((gst_color_balance_get_value (balance, c)
                - c->min_value) * 100) /
              (c->max_value - c->min_value);

          break;
        }
    }

  g_object_unref (balance);

  return percent;
}


guint
empathy_video_src_get_supported_channels (GstElement *src)
{
  GstColorBalance *balance;
  const GList *channels;
  GList *l;
  guint result = 0;

  balance = dup_color_balance (src);
  if (balance == NULL)
    goto out;

  channels = gst_color_balance_list_channels (balance);

  for (l = (GList *) channels; l != NULL; l = g_list_next (l))
    {
      GstColorBalanceChannel *channel = GST_COLOR_BALANCE_CHANNEL (l->data);
      int i;

      for (i = 0; i < NR_EMPATHY_GST_VIDEO_SRC_CHANNELS; i++)
        {
          if (g_ascii_strcasecmp (channel->label, channel_names[i]) == 0)
            {
              result |= (1 << i);
              break;
            }
        }
    }

  g_object_unref (balance);

out:
  return result;
}

void
empathy_video_src_change_device (EmpathyGstVideoSrc *self,
    const gchar *device)
{
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (self);
  GstState state;

  gst_element_get_state (priv->src, &state, NULL, 0);

  g_return_if_fail (state == GST_STATE_NULL);

  g_object_set (priv->src, "device", device, NULL);
}

gchar *
empathy_video_src_dup_device (EmpathyGstVideoSrc *self)
{
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (self);
  gchar *device;

  g_object_get (priv->src, "device", &device, NULL);

  return device;
}

void
empathy_video_src_set_framerate (GstElement *src,
    guint framerate)
{
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (src);

  if (priv->videorate)
    {
      g_object_set (G_OBJECT (priv->videorate), "max-rate", framerate, NULL);
    }
}

void
empathy_video_src_set_resolution (GstElement *src,
    guint width,
    guint height)
{
  EmpathyGstVideoSrcPrivate *priv = EMPATHY_GST_VIDEO_SRC_GET_PRIVATE (src);
  GstCaps *caps;
  GstPad *srcpad, *peer;

  g_return_if_fail (priv->capsfilter != NULL);

  gst_element_set_locked_state (priv->src, TRUE);
  gst_element_set_state (priv->src, GST_STATE_NULL);

  srcpad = gst_element_get_static_pad (priv->src, "src");
  peer = gst_pad_get_peer (srcpad);

  /* Keep a ref as removing it from the bin will loose our reference */
  gst_object_ref (priv->src);
  gst_bin_remove (GST_BIN (src), priv->src);

  g_object_get (priv->capsfilter, "caps", &caps, NULL);
  caps = gst_caps_make_writable (caps);

  gst_caps_set_simple (caps,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      NULL);

  g_object_set (priv->capsfilter, "caps", caps, NULL);
  gst_caps_unref (caps);

  gst_bin_add (GST_BIN (src), priv->src);
  /* We as the bin own the source again, so drop the temporary ref */
  gst_object_unref (priv->src);

  gst_pad_link (srcpad, peer);

  gst_element_set_locked_state (priv->src, FALSE);
  gst_element_sync_state_with_parent (priv->src);

  gst_object_unref (srcpad);
  gst_object_unref (peer);
}
