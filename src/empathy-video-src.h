/*
 * empathy-gst-video-src.h - Header for EmpathyGstVideoSrc
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

#ifndef __EMPATHY_GST_VIDEO_SRC_H__
#define __EMPATHY_GST_VIDEO_SRC_H__

#include <glib-object.h>
#include <gst/gst.h>

G_BEGIN_DECLS

typedef struct _EmpathyGstVideoSrc EmpathyGstVideoSrc;
typedef struct _EmpathyGstVideoSrcClass EmpathyGstVideoSrcClass;

typedef enum {
  EMPATHY_GST_VIDEO_SRC_CHANNEL_CONTRAST = 0,
  EMPATHY_GST_VIDEO_SRC_CHANNEL_BRIGHTNESS = 1,
  EMPATHY_GST_VIDEO_SRC_CHANNEL_GAMMA = 2,
  NR_EMPATHY_GST_VIDEO_SRC_CHANNELS
} EmpathyGstVideoSrcChannel;

#define  EMPATHY_GST_VIDEO_SRC_SUPPORTS_CONTRAST \
  (1 << EMPATHY_GST_VIDEO_SRC_CHANNEL_CONTRAST)
#define  EMPATHY_GST_VIDEO_SRC_SUPPORTS_BRIGHTNESS \
  (1 << EMPATHY_GST_VIDEO_SRC_CHANNEL_BRIGHTNESS)
#define  EMPATHY_GST_VIDEO_SRC_SUPPORTS_GAMMA \
  (1 << EMPATHY_GST_VIDEO_SRC_CHANNEL_GAMMA)

struct _EmpathyGstVideoSrcClass {
    GstBinClass parent_class;
};

struct _EmpathyGstVideoSrc {
    GstBin parent;
};

GType empathy_video_src_get_type (void);

/* TYPE MACROS */
#define EMPATHY_TYPE_GST_VIDEO_SRC \
  (empathy_video_src_get_type ())
#define EMPATHY_GST_VIDEO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EMPATHY_TYPE_GST_VIDEO_SRC, \
    EmpathyGstVideoSrc))
#define EMPATHY_GST_VIDEO_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EMPATHY_TYPE_GST_VIDEO_SRC, \
    EmpathyGstVideoSrcClass))
#define EMPATHY_IS_GST_VIDEO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EMPATHY_TYPE_GST_VIDEO_SRC))
#define EMPATHY_IS_GST_VIDEO_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EMPATHY_TYPE_GST_VIDEO_SRC))
#define EMPATHY_GST_VIDEO_SRC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EMPATHY_TYPE_GST_VIDEO_SRC, \
    EmpathyGstVideoSrcClass))

GstElement *empathy_video_src_new (void);

guint
empathy_video_src_get_supported_channels (GstElement *src);

void empathy_video_src_set_channel (GstElement *src,
  EmpathyGstVideoSrcChannel channel, guint percent);

guint empathy_video_src_get_channel (GstElement *src,
  EmpathyGstVideoSrcChannel channel);

void empathy_video_src_change_device (EmpathyGstVideoSrc *self,
  const gchar *device);
gchar * empathy_video_src_dup_device (EmpathyGstVideoSrc *self);

void empathy_video_src_set_framerate (GstElement *src,
    guint framerate);

void empathy_video_src_set_resolution (GstElement *src,
    guint width, guint height);

G_END_DECLS

#endif /* #ifndef __EMPATHY_GST_VIDEO_SRC_H__*/
