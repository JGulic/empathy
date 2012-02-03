/*
 * empathy-streamed-media-window-fullscreen.c - Source for EmpathyStreamedMediaWindowFullscreen
 * Copyright (C) 2009 Collabora Ltd.
 *
 * Some code is based on the Totem Movie Player, especially
 * totem-fullscreen.c which has the following copyright:
 * Copyright (C) 2001-2007 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2007 Sunil Mohan Adapa <sunilmohan@gnu.org.in>
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

#include "empathy-streamed-media-window-fullscreen.h"

#include <gtk/gtk.h>

#include <libempathy/empathy-utils.h>
#include <libempathy-gtk/empathy-ui-utils.h>

/* The number of seconds for which the "leave fullscreen" popup should
   be shown */
#define FULLSCREEN_POPUP_TIMEOUT 5

G_DEFINE_TYPE (EmpathyStreamedMediaWindowFullscreen, empathy_streamed_media_window_fullscreen,
    G_TYPE_OBJECT)

/* private structure */
typedef struct _EmpathyStreamedMediaWindowFullscreenPriv
    EmpathyStreamedMediaWindowFullscreenPriv;

struct _EmpathyStreamedMediaWindowFullscreenPriv
{
  EmpathyStreamedMediaWindow *parent_window;

  GtkWidget *leave_fullscreen_popup;
  GtkWidget *video_widget;

  guint popup_timeout;
  gboolean popup_creation_in_progress;
  gboolean dispose_has_run;
};

#define GET_PRIV(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EMPATHY_TYPE_STREAMED_MEDIA_WINDOW_FULLSCREEN, \
    EmpathyStreamedMediaWindowFullscreenPriv))

static void empathy_streamed_media_window_fullscreen_dispose (GObject *object);
static void empathy_streamed_media_window_fullscreen_finalize (GObject *object);

static gboolean empathy_streamed_media_window_fullscreen_hide_popup (
    EmpathyStreamedMediaWindowFullscreen *fs);

static void
empathy_streamed_media_window_fullscreen_set_cursor_visible (
    EmpathyStreamedMediaWindowFullscreen *fs,
    gboolean show_cursor)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (fs);
  GdkWindow *window;

  if (priv->video_widget == NULL)
    return;

  window = gtk_widget_get_window (priv->video_widget);

  if (!show_cursor)
    gdk_window_set_cursor (window, gdk_cursor_new (GDK_BLANK_CURSOR));
  else
    gdk_window_set_cursor (window, NULL);
}

static void
empathy_streamed_media_window_fullscreen_add_popup_timeout (
    EmpathyStreamedMediaWindowFullscreen *self)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  if (priv->popup_timeout == 0)
    {
      priv->popup_timeout = g_timeout_add_seconds (FULLSCREEN_POPUP_TIMEOUT,
          (GSourceFunc) empathy_streamed_media_window_fullscreen_hide_popup, self);
    }
}

static void
empathy_streamed_media_window_fullscreen_remove_popup_timeout (
    EmpathyStreamedMediaWindowFullscreen *self)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  if (priv->popup_timeout != 0)
    {
      g_source_remove (priv->popup_timeout);
      priv->popup_timeout = 0;
    }
}

void
empathy_streamed_media_window_fullscreen_show_popup (EmpathyStreamedMediaWindowFullscreen *self)
{
  gint leave_fullscreen_width, leave_fullscreen_height;
  GdkScreen *screen;
  GdkRectangle fullscreen_rect;
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  g_assert (self->is_fullscreen);

  g_return_if_fail (priv->parent_window != NULL);

  if (priv->popup_creation_in_progress)
    return;

  if (!gtk_window_is_active (GTK_WINDOW (priv->parent_window)))
    return;

  priv->popup_creation_in_progress = TRUE;

  empathy_streamed_media_window_fullscreen_set_cursor_visible (self, TRUE);

  /* Obtaining the screen rectangle */
  screen = gtk_window_get_screen (GTK_WINDOW (priv->parent_window));
  gdk_screen_get_monitor_geometry (screen,
      gdk_screen_get_monitor_at_window (screen,
          gtk_widget_get_window (GTK_WIDGET (priv->parent_window))),
      &fullscreen_rect);

  /* Getting the popup window sizes */
  gtk_window_get_size (GTK_WINDOW (priv->leave_fullscreen_popup),
      &leave_fullscreen_width, &leave_fullscreen_height);

  /* Moving the popup to the top-right corner (if the direction is LTR) or the
     top-left corner (if the direction is RTL).*/
  if (gtk_widget_get_direction (priv->leave_fullscreen_popup)
        == GTK_TEXT_DIR_LTR)
    {
      gtk_window_move (GTK_WINDOW (priv->leave_fullscreen_popup),
          fullscreen_rect.width + fullscreen_rect.x - leave_fullscreen_width,
          fullscreen_rect.y);

    }
  else
    {
      gtk_window_move (GTK_WINDOW (priv->leave_fullscreen_popup),
          fullscreen_rect.x, fullscreen_rect.y);
    }

  gtk_widget_show_all (priv->leave_fullscreen_popup);
  empathy_streamed_media_window_fullscreen_add_popup_timeout (self);

  priv->popup_creation_in_progress = FALSE;
}

static gboolean
empathy_streamed_media_window_fullscreen_hide_popup (EmpathyStreamedMediaWindowFullscreen *fs)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (fs);

  if (priv->video_widget == NULL || !fs->is_fullscreen)
    return TRUE;

  gtk_widget_hide (priv->leave_fullscreen_popup);
  empathy_streamed_media_window_fullscreen_remove_popup_timeout (fs);

  empathy_streamed_media_window_fullscreen_set_cursor_visible (fs, FALSE);

  return FALSE;
}

static void
empathy_streamed_media_window_fullscreen_init (EmpathyStreamedMediaWindowFullscreen *self)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);
  GtkBuilder *gui;
  gchar *filename;

  filename = empathy_file_lookup ("empathy-call-window-fullscreen.ui", "src");
  gui = empathy_builder_get_file (filename,
    "leave_fullscreen_window", &priv->leave_fullscreen_popup,
    "leave_fullscreen_button", &self->leave_fullscreen_button,
    NULL);

  gtk_widget_add_events (priv->leave_fullscreen_popup, GDK_POINTER_MOTION_MASK);

  g_object_unref (gui);
  g_free (filename);
}

static void
empathy_streamed_media_window_fullscreen_class_init (
    EmpathyStreamedMediaWindowFullscreenClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (EmpathyStreamedMediaWindowFullscreenPriv));

  object_class->dispose = empathy_streamed_media_window_fullscreen_dispose;
  object_class->finalize = empathy_streamed_media_window_fullscreen_finalize;
}

void
empathy_streamed_media_window_fullscreen_dispose (GObject *object)
{
  EmpathyStreamedMediaWindowFullscreen *self = EMPATHY_STREAMED_MEDIA_WINDOW_FULLSCREEN (object);
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (priv->leave_fullscreen_popup != NULL)
    gtk_widget_destroy (priv->leave_fullscreen_popup);
  priv->leave_fullscreen_popup = NULL;

  if (G_OBJECT_CLASS (empathy_streamed_media_window_fullscreen_parent_class)->dispose)
    {
      G_OBJECT_CLASS (
          empathy_streamed_media_window_fullscreen_parent_class)->dispose (object);
    }
}

void
empathy_streamed_media_window_fullscreen_finalize (GObject *object)
{
  EmpathyStreamedMediaWindowFullscreen *self = EMPATHY_STREAMED_MEDIA_WINDOW_FULLSCREEN (object);

  empathy_streamed_media_window_fullscreen_remove_popup_timeout (self);

  G_OBJECT_CLASS (
      empathy_streamed_media_window_fullscreen_parent_class)->finalize (object);
}

static void
empathy_streamed_media_window_fullscreen_parent_window_notify (GtkWidget *parent_window,
    GParamSpec *property, EmpathyStreamedMediaWindowFullscreen *fs)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (fs);

  if (!fs->is_fullscreen)
    return;

  if (parent_window == GTK_WIDGET (priv->parent_window) &&
        !gtk_window_is_active (GTK_WINDOW (parent_window)))
    {
      empathy_streamed_media_window_fullscreen_hide_popup (fs);
      empathy_streamed_media_window_fullscreen_set_cursor_visible (fs, TRUE);
    }
}

EmpathyStreamedMediaWindowFullscreen *
empathy_streamed_media_window_fullscreen_new (EmpathyStreamedMediaWindow *parent_window)
{
  EmpathyStreamedMediaWindowFullscreen *self = EMPATHY_STREAMED_MEDIA_WINDOW_FULLSCREEN (
    g_object_new (EMPATHY_TYPE_STREAMED_MEDIA_WINDOW_FULLSCREEN, NULL));
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  priv->parent_window = parent_window;
  g_signal_connect (G_OBJECT (priv->parent_window), "notify::is-active",
    G_CALLBACK (empathy_streamed_media_window_fullscreen_parent_window_notify), self);

  return self;
}

void
empathy_streamed_media_window_fullscreen_set_fullscreen (EmpathyStreamedMediaWindowFullscreen *fs,
  gboolean set_fullscreen)
{

  if (set_fullscreen)
      empathy_streamed_media_window_fullscreen_remove_popup_timeout (fs);
  else
      empathy_streamed_media_window_fullscreen_hide_popup (fs);

  empathy_streamed_media_window_fullscreen_set_cursor_visible (fs, !set_fullscreen);
  fs->is_fullscreen = set_fullscreen;
}

static void
video_widget_destroy_cb (GtkWidget *widget,
    EmpathyStreamedMediaWindowFullscreen *self)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (self);

  priv->video_widget = NULL;
}

void
empathy_streamed_media_window_fullscreen_set_video_widget (
    EmpathyStreamedMediaWindowFullscreen *fs,
    GtkWidget *video_widget)
{
  EmpathyStreamedMediaWindowFullscreenPriv *priv = GET_PRIV (fs);
  priv->video_widget = video_widget;

  tp_g_signal_connect_object (video_widget, "destroy",
      G_CALLBACK (video_widget_destroy_cb), fs, 0);
}
