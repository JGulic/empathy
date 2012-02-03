/*
 * Copyright (C) 2006-2007 Imendio AB.
 * Copyright (C) 2007-2008 Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Based on Novell's e-image-chooser.
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <libempathy/empathy-camera-monitor.h>
#include <libempathy/empathy-gsettings.h>
#include <libempathy/empathy-utils.h>

#include "empathy-avatar-chooser.h"
#include "empathy-images.h"
#include "empathy-ui-utils.h"

#ifdef HAVE_CHEESE
#include <cheese-avatar-chooser.h>
#endif /* HAVE_CHEESE */


#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>

/**
 * SECTION:empathy-avatar-chooser
 * @title: EmpathyAvatarChooser
 * @short_description: A widget used to change avatar
 * @include: libempathy-gtk/empathy-avatar-chooser.h
 *
 * #EmpathyAvatarChooser is a widget which extends #GtkButton to
 * provide a way of changing avatar.
 */

/**
 * EmpathyAvatarChooser:
 * @parent: parent object
 *
 * Widget which extends #GtkButton to provide a way of changing avatar.
 */

#define AVATAR_SIZE_SAVE 96
#define AVATAR_SIZE_VIEW 64
#define DEFAULT_DIR DATADIR"/pixmaps/faces"

#ifdef HAVE_CHEESE
/*
 * A custom GtkResponseType used when the user presses the
 * "Camera Picture" button. Any positive value would be sufficient.
 */
#define EMPATHY_AVATAR_CHOOSER_RESPONSE_WEBCAM   10
#endif
#define EMPATHY_AVATAR_CHOOSER_RESPONSE_NO_IMAGE GTK_RESPONSE_NO
#define EMPATHY_AVATAR_CHOOSER_RESPONSE_CANCEL   GTK_RESPONSE_CANCEL
#define EMPATHY_AVATAR_CHOOSER_RESPONSE_FILE     GTK_RESPONSE_OK

struct _EmpathyAvatarChooserPrivate
{
  TpConnection *connection;
  GtkFileChooser *chooser_dialog;

  gulong ready_handler_id;

  EmpathyAvatar *avatar;
  GSettings *gsettings_ui;
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_CONNECTION
};

static guint signals [LAST_SIGNAL];

G_DEFINE_TYPE (EmpathyAvatarChooser, empathy_avatar_chooser, GTK_TYPE_BUTTON);

/*
 * Drag and drop stuff
 */
#define URI_LIST_TYPE "text/uri-list"

enum DndTargetType
{
  DND_TARGET_TYPE_URI_LIST
};

static const GtkTargetEntry drop_types[] =
{
  { URI_LIST_TYPE, 0, DND_TARGET_TYPE_URI_LIST },
};

static void
avatar_chooser_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyAvatarChooser *self = (EmpathyAvatarChooser *) object;

  switch (param_id)
    {
      case PROP_CONNECTION:
        g_value_set_object (value, self->priv->connection);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
avatar_chooser_set_connection (EmpathyAvatarChooser *self,
    TpConnection *connection)
{
  tp_clear_object (&self->priv->connection);

  if (connection != NULL)
    self->priv->connection = g_object_ref (connection);
}

static void
avatar_chooser_set_property (GObject *object,
    guint param_id,
    const GValue *value,
    GParamSpec *pspec)
{
  EmpathyAvatarChooser *self = EMPATHY_AVATAR_CHOOSER (object);

  switch (param_id)
    {
      case PROP_CONNECTION:
        avatar_chooser_set_connection (self, g_value_get_object (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
        break;
    }
}

static void
avatar_chooser_dispose (GObject *object)
{
  EmpathyAvatarChooser *self = (EmpathyAvatarChooser *) object;

  tp_clear_object (&self->priv->connection);
  tp_clear_pointer (&self->priv->avatar, empathy_avatar_unref);
  tp_clear_object (&self->priv->gsettings_ui);

  G_OBJECT_CLASS (empathy_avatar_chooser_parent_class)->dispose (object);
}

static void
empathy_avatar_chooser_class_init (EmpathyAvatarChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GParamSpec *param_spec;

  object_class->dispose = avatar_chooser_dispose;
  object_class->get_property = avatar_chooser_get_property;
  object_class->set_property = avatar_chooser_set_property;

  /**
   * EmpathyAvatarChooser::changed:
   * @self: an #EmpathyAvatarChooser
   *
   * Emitted when the chosen avatar has changed.
   *
   */
  signals[CHANGED] =
    g_signal_new ("changed",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
            0,
            NULL, NULL,
            g_cclosure_marshal_generic,
            G_TYPE_NONE, 0);

  /**
   * EmpathyAvatarChooser:connection:
   *
   * The #TpConnection whose avatar should be shown and modified by
   * the #EmpathyAvatarChooser instance.
   */
  param_spec = g_param_spec_object ("connection",
            "TpConnection",
            "TpConnection whose avatar should be "
            "shown and modified by this widget",
            TP_TYPE_CONNECTION,
            G_PARAM_READWRITE |
            G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (object_class,
           PROP_CONNECTION,
           param_spec);

  g_type_class_add_private (object_class, sizeof (EmpathyAvatarChooserPrivate));
}

static gboolean
avatar_chooser_drag_motion_cb (GtkWidget *widget,
    GdkDragContext *context,
    gint x,
    gint y,
    guint time_,
    EmpathyAvatarChooser *self)
{
  GList *p;

  for (p = gdk_drag_context_list_targets (context); p != NULL;
       p = p->next)
    {
      gchar *possible_type;

      possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));

      if (!strcmp (possible_type, URI_LIST_TYPE))
        {
          g_free (possible_type);
          gdk_drag_status (context, GDK_ACTION_COPY, time_);

          return TRUE;
        }

      g_free (possible_type);
    }

  return FALSE;
}

static gboolean
avatar_chooser_drag_drop_cb (GtkWidget *widget,
    GdkDragContext *context,
    gint x,
    gint y,
    guint time_,
    EmpathyAvatarChooser *self)
{
  GList *p;

  if (gdk_drag_context_list_targets (context) == NULL)
    return FALSE;

  for (p = gdk_drag_context_list_targets (context);
       p != NULL; p = p->next)
    {
      char *possible_type;

      possible_type = gdk_atom_name (GDK_POINTER_TO_ATOM (p->data));
      if (!strcmp (possible_type, URI_LIST_TYPE))
        {
          g_free (possible_type);
          gtk_drag_get_data (widget, context,
                 GDK_POINTER_TO_ATOM (p->data),
                 time_);

          return TRUE;
        }

      g_free (possible_type);
    }

  return FALSE;
}

static void
avatar_chooser_clear_image (EmpathyAvatarChooser *self)
{
  GtkWidget *image;

  tp_clear_pointer (&self->priv->avatar, empathy_avatar_unref);

  image = gtk_image_new_from_icon_name (EMPATHY_IMAGE_AVATAR_DEFAULT,
    GTK_ICON_SIZE_DIALOG);
  gtk_button_set_image (GTK_BUTTON (self), image);
  g_signal_emit (self, signals[CHANGED], 0);
}

static gboolean
str_in_strv (const gchar  *str,
    gchar **strv)
{
  if (strv == NULL)
    return FALSE;

  while (*strv != NULL)
    {
      if (g_str_equal (str, *strv))
        return TRUE;

      strv++;
    }

  return FALSE;
}

/* The caller must free the strings stored in satisfactory_format_name and
 * satisfactory_mime_type.
 */
static gboolean
avatar_chooser_need_mime_type_conversion (const gchar *current_mime_type,
    gchar **accepted_mime_types,
    gchar **satisfactory_format_name,
    gchar **satisfactory_mime_type)
{
  gchar *good_mime_types[] = {"image/jpeg", "image/png", NULL};
  guint i;
  GSList *formats, *l;
  gboolean found = FALSE;

  *satisfactory_format_name = NULL;
  *satisfactory_mime_type = NULL;

  /* If there is no accepted format there is nothing we can do */
  if (accepted_mime_types == NULL || *accepted_mime_types == NULL)
    return TRUE;

  /* If the current mime type is good and accepted, don't change it!
   * jpeg is compress better pictures, but png is better for logos and
   * could have an alpha layer. */
  if (str_in_strv (current_mime_type, good_mime_types) &&
      str_in_strv (current_mime_type, accepted_mime_types))
    {
      *satisfactory_mime_type = g_strdup (current_mime_type);
      *satisfactory_format_name = g_strdup (current_mime_type +
                    strlen ("image/"));
      return FALSE;
    }

  /* The current mime type is either not accepted or not good to use.
   * Check if one of the good format is supported... */
  for (i = 0; good_mime_types[i] != NULL;  i++)
    {
      if (str_in_strv (good_mime_types[i], accepted_mime_types))
        {
          *satisfactory_mime_type = g_strdup (good_mime_types[i]);
          *satisfactory_format_name = g_strdup (good_mime_types[i] +
              strlen ("image/"));
          return TRUE;
        }
    }

  /* Pick the first supported format we can write */
  formats = gdk_pixbuf_get_formats ();
  for (l = formats; !found && l != NULL; l = l->next)
    {
      GdkPixbufFormat *format = l->data;
      gchar **format_mime_types;
      gchar **iter;

      if (!gdk_pixbuf_format_is_writable (format))
        continue;

      format_mime_types = gdk_pixbuf_format_get_mime_types (format);
      for (iter = format_mime_types; *iter != NULL; iter++)
        {
          if (str_in_strv (*iter, accepted_mime_types))
            {
              *satisfactory_format_name = gdk_pixbuf_format_get_name (format);
              *satisfactory_mime_type = g_strdup (*iter);
              found = TRUE;
              break;
            }
        }
      g_strfreev (format_mime_types);
    }
  g_slist_free (formats);

  return TRUE;
}

static void
avatar_chooser_error_show (EmpathyAvatarChooser *self,
    const gchar *primary_text,
    const gchar *secondary_text)
{
  GtkWidget *parent;
  GtkWidget *dialog;

  parent = gtk_widget_get_toplevel (GTK_WIDGET (self));
  if (!GTK_IS_WINDOW (parent))
    parent = NULL;

  dialog = gtk_message_dialog_new (parent ? GTK_WINDOW (parent) : NULL,
      GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE,
      "%s", primary_text);

  if (secondary_text != NULL)
    {
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
          "%s", secondary_text);
    }

  g_signal_connect (dialog, "response",
        G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_widget_show (dialog);

}

static EmpathyAvatar *
avatar_chooser_maybe_convert_and_scale (EmpathyAvatarChooser *self,
    GdkPixbuf *pixbuf,
    EmpathyAvatar *avatar)
{
  TpAvatarRequirements *req;
  gboolean needs_conversion = FALSE;
  guint width, height;
  gchar *new_format_name = NULL;
  gchar *new_mime_type = NULL;
  gdouble min_factor, max_factor;
  gdouble factor;
  gchar *best_image_data = NULL;
  gsize best_image_size = 0;
  guint count = 0;

  req = tp_connection_get_avatar_requirements (self->priv->connection);
  if (req == NULL)
    {
      DEBUG ("Avatar requirements not ready");
      return NULL;
    }

  /* Smaller is the factor, smaller will be the image.
   * 0 is an empty image, 1 is the full size. */
  min_factor = 0;
  max_factor = 1;
  factor = 1;

  /* Check if we need to convert to another image format */
  if (avatar_chooser_need_mime_type_conversion (avatar->format,
        req->supported_mime_types, &new_format_name, &new_mime_type))
    {
      DEBUG ("Format conversion needed, we'll use mime type '%s' "
             "and format name '%s'. Current mime type is '%s'",
             new_mime_type, new_format_name, avatar->format);
      needs_conversion = TRUE;
    }

  /* If there is no format we can use, report error to the user. */
  if (new_mime_type == NULL || new_format_name == NULL)
    {
      avatar_chooser_error_show (self, _("Couldn't convert image"),
          _("None of the accepted image formats are "
            "supported on your system"));
      return NULL;
    }

  /* If width or height are too big, it needs converting. */
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  if ((req->maximum_width > 0 && width > req->maximum_width) ||
      (req->maximum_height > 0 && height > req->maximum_height))
    {
      gdouble h_factor, v_factor;

      h_factor = (gdouble) req->maximum_width / width;
      v_factor = (gdouble) req->maximum_height / height;
      factor = max_factor = MIN (h_factor, v_factor);

      DEBUG ("Image dimensions (%dx%d) are too big. Max is %dx%d.",
          width, height, req->maximum_width, req->maximum_height);

      needs_conversion = TRUE;
    }

  /* If the data len is too big and no other conversion is needed,
   * try with a lower factor. */
  if (req->maximum_bytes > 0 && avatar->len > req->maximum_bytes &&
      !needs_conversion)
    {
      DEBUG ("Image data (%"G_GSIZE_FORMAT" bytes) is too big "
             "(max is %u bytes), conversion needed.",
             avatar->len, req->maximum_bytes);

      factor = 0.5;
      needs_conversion = TRUE;
    }

  /* If no conversion is needed, return the avatar */
  if (!needs_conversion)
    {
      g_free (new_format_name);
      g_free (new_mime_type);
      return empathy_avatar_ref (avatar);
    }

  do
    {
      GdkPixbuf *pixbuf_scaled = NULL;
      gboolean saved;
      gint new_width, new_height;
      gchar *converted_image_data;
      gsize converted_image_size;
      GError *error = NULL;

      if (factor != 1)
        {
          new_width = width * factor;
          new_height = height * factor;
          pixbuf_scaled = gdk_pixbuf_scale_simple (pixbuf,
                     new_width,
                     new_height,
                     GDK_INTERP_HYPER);
        }
      else
        {
          new_width = width;
          new_height = height;
          pixbuf_scaled = g_object_ref (pixbuf);
        }

      DEBUG ("Trying with factor %f (%dx%d) and format %s...", factor,
        new_width, new_height, new_format_name);

      saved = gdk_pixbuf_save_to_buffer (pixbuf_scaled,
          &converted_image_data,
          &converted_image_size,
          new_format_name,
          &error, NULL);
      g_object_unref (pixbuf_scaled);

      if (!saved)
        {
          g_free (new_format_name);
          g_free (new_mime_type);
          avatar_chooser_error_show (self,
            _("Couldn't convert image"),
            error ? error->message : NULL);
          g_clear_error (&error);
          return NULL;
        }

      DEBUG ("Produced an image data of %"G_GSIZE_FORMAT" bytes.",
        converted_image_size);

      /* If the new image satisfy the req, keep it as current best */
      if (req->maximum_bytes == 0 || converted_image_size <= req->maximum_bytes)
        {
          g_free (best_image_data);

          best_image_data = converted_image_data;
          best_image_size = converted_image_size;

          /* If this image is close enough to the optimal size,
           * stop searching */
          if (req->maximum_bytes == 0 ||
              req->maximum_bytes - converted_image_size <= 1024)
            break;
        }
      else
        {
          g_free (converted_image_data);
        }

    /* Make a binary search for the bigest factor that produce
     * an image data size less than max_size */
    if (converted_image_size > req->maximum_bytes)
      max_factor = factor;
    if (converted_image_size < req->maximum_bytes)
      min_factor = factor;
    factor = (min_factor + max_factor)/2;

    if ((int) (width * factor) == new_width ||
        (int) (height * factor) == new_height)
      {
        /* min_factor and max_factor are too close, so the new
         * factor will produce the same image as previous
         * iteration. No need to continue, we already found
         * the optimal size. */
        break;
      }

    /* Do 10 iterations in the worst case */
  } while (++count < 10);

  g_free (new_format_name);

  avatar = empathy_avatar_new ((guchar *) best_image_data,
    best_image_size, new_mime_type, NULL);

  g_free (best_image_data);
  g_free (new_mime_type);

  return avatar;
}

static void
avatar_chooser_set_image (EmpathyAvatarChooser *self,
    EmpathyAvatar *avatar,
    GdkPixbuf *pixbuf,
    gboolean set_locally)
{
  GdkPixbuf *pixbuf_view;
  GtkWidget *image;

  g_assert (avatar != NULL);
  g_assert (pixbuf != NULL);

  if (set_locally)
    {
      EmpathyAvatar *conv;

      conv = avatar_chooser_maybe_convert_and_scale (self,
        pixbuf, avatar);
      empathy_avatar_unref (avatar);

      if (conv == NULL)
        /* An error occured; don't change the avatar. */
        return;

      avatar = conv;
    }

  tp_clear_pointer (&self->priv->avatar, empathy_avatar_unref);
  self->priv->avatar = avatar;

  pixbuf_view = empathy_pixbuf_scale_down_if_necessary (pixbuf,
      AVATAR_SIZE_VIEW);
  image = gtk_image_new_from_pixbuf (pixbuf_view);

  gtk_button_set_image (GTK_BUTTON (self), image);
  g_signal_emit (self, signals[CHANGED], 0);

  g_object_unref (pixbuf_view);
  g_object_unref (pixbuf);
}

/* takes ownership of @data */
static void
avatar_chooser_set_image_from_data (EmpathyAvatarChooser *self,
    gchar *data,
    gsize size,
    gboolean set_locally)
{
  GdkPixbuf *pixbuf;
  EmpathyAvatar *avatar = NULL;
  gchar *mime_type = NULL;

  if (data == NULL)
    {
      avatar_chooser_clear_image (self);
      return;
    }

  pixbuf = empathy_pixbuf_from_data_and_mime (data, size, &mime_type);
  if (pixbuf == NULL)
    {
      g_free (data);
      data = NULL;
      return;
    }

  avatar = empathy_avatar_new ((guchar *) data, size, mime_type, NULL);

  avatar_chooser_set_image (self, avatar, pixbuf, set_locally);

  g_free (mime_type);
  g_free (data);
}

static void
avatar_chooser_drag_data_received_cb (GtkWidget          *widget,
    GdkDragContext *context,
    gint x,
    gint y,
    GtkSelectionData *selection_data,
    guint info,
    guint time_,
    EmpathyAvatarChooser *self)
{
  gchar *target_type;
  gboolean handled = FALSE;

  target_type = gdk_atom_name (gtk_selection_data_get_target (selection_data));
  if (!strcmp (target_type, URI_LIST_TYPE))
    {
      GFile *file;
      gchar *nl;
      gchar *data = NULL;
      gsize bytes_read;

      nl = strstr ((gchar *) gtk_selection_data_get_data (selection_data),
              "\r\n");
      if (nl != NULL)
        {
          gchar *uri;

          uri = g_strndup (
              (gchar *) gtk_selection_data_get_data (selection_data),
              nl - (gchar *) gtk_selection_data_get_data (selection_data));

          file = g_file_new_for_uri (uri);
          g_free (uri);
        }
      else
        {
          file = g_file_new_for_uri ((gchar *) gtk_selection_data_get_data (
                selection_data));
        }

      handled = g_file_load_contents (file, NULL, &data, &bytes_read,
              NULL, NULL);

      if (handled)
        {
          /* pass data to the avatar_chooser_set_image_from_data */
          avatar_chooser_set_image_from_data (self, data, bytes_read, TRUE);
        }

      g_object_unref (file);
    }

  gtk_drag_finish (context, handled, FALSE, time_);
}

static void
avatar_chooser_update_preview_cb (GtkFileChooser *file_chooser,
    EmpathyAvatarChooser *self)
{
  gchar *filename;

  filename = gtk_file_chooser_get_preview_filename (file_chooser);

  if (filename != NULL)
    {
      GtkWidget *image;
      GdkPixbuf *pixbuf = NULL;
      GdkPixbuf *scaled_pixbuf;

      pixbuf = gdk_pixbuf_new_from_file (filename, NULL);

      image = gtk_file_chooser_get_preview_widget (file_chooser);

      if (pixbuf != NULL)
        {
          scaled_pixbuf = empathy_pixbuf_scale_down_if_necessary (pixbuf,
              AVATAR_SIZE_SAVE);

          gtk_image_set_from_pixbuf (GTK_IMAGE (image), scaled_pixbuf);
          g_object_unref (scaled_pixbuf);
          g_object_unref (pixbuf);
        }
      else
        {
          gtk_image_set_from_stock (GTK_IMAGE (image),
                  "gtk-dialog-question",
                  GTK_ICON_SIZE_DIALOG);
        }

      g_free (filename);
    }

  gtk_file_chooser_set_preview_widget_active (file_chooser, TRUE);
}

static void
avatar_chooser_set_image_from_file (EmpathyAvatarChooser *self,
    const gchar *filename)
{
  gchar *image_data = NULL;
  gsize  image_size = 0;
  GError *error = NULL;

  if (!g_file_get_contents (filename, &image_data, &image_size, &error))
    {
      DEBUG ("Failed to load image from '%s': %s", filename,
        error ? error->message : "No error given");

      g_clear_error (&error);
      return;
    }

  /* pass image_data to the avatar_chooser_set_image_from_data */
  avatar_chooser_set_image_from_data (self, image_data, image_size, TRUE);
}

#ifdef HAVE_CHEESE
static void
avatar_chooser_set_avatar_from_pixbuf (EmpathyAvatarChooser *self,
               GdkPixbuf *pb)
{
  gsize size;
  gchar *buf;
  EmpathyAvatar *avatar = NULL;
  GError *error = NULL;

  if (!gdk_pixbuf_save_to_buffer (pb, &buf, &size, "png", &error, NULL))
    {
      avatar_chooser_error_show (self,
        _("Couldn't save picture to file"),
        error ? error->message : NULL);
      g_clear_error (&error);
      return;
    }

  avatar = empathy_avatar_new ((guchar *) buf, size, "image/png", NULL);
  avatar_chooser_set_image (self, avatar, pb, TRUE);

  g_free (buf);
}

static gboolean
destroy_chooser (GtkWidget *self)
{
  gtk_widget_destroy (self);
  return FALSE;
}

static void
webcam_response_cb (GtkDialog *dialog,
        int response,
        EmpathyAvatarChooser *self)
{
  if (response == GTK_RESPONSE_ACCEPT)
    {
      GdkPixbuf *pb;
      CheeseAvatarChooser *cheese_chooser;

      cheese_chooser = CHEESE_AVATAR_CHOOSER (dialog);
      pb = cheese_avatar_chooser_get_picture (cheese_chooser);
      avatar_chooser_set_avatar_from_pixbuf (self, pb);
    }

  if (response != GTK_RESPONSE_DELETE_EVENT &&
      response != GTK_RESPONSE_NONE)
    g_idle_add ((GSourceFunc) destroy_chooser, dialog);
}

static void
choose_avatar_from_webcam (GtkWidget *widget,
    EmpathyAvatarChooser *self)
{
  GtkWidget *window;

  window = cheese_avatar_chooser_new ();

  gtk_window_set_transient_for (GTK_WINDOW (window),
      GTK_WINDOW (empathy_get_toplevel_window (GTK_WIDGET (self))));
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
  g_signal_connect (G_OBJECT (window), "response",
      G_CALLBACK (webcam_response_cb), self);
  gtk_widget_show (window);
}
#endif /* HAVE_CHEESE */

static void
avatar_chooser_response_cb (GtkWidget *widget,
    gint response,
    EmpathyAvatarChooser *self)
{
  self->priv->chooser_dialog = NULL;

  if (response == EMPATHY_AVATAR_CHOOSER_RESPONSE_FILE)
    {
      gchar *filename;
      gchar *path;

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (widget));
      avatar_chooser_set_image_from_file (self, filename);
      g_free (filename);

      path = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (widget));
      if (path != NULL)
        {
          g_settings_set_string (self->priv->gsettings_ui,
                     EMPATHY_PREFS_UI_AVATAR_DIRECTORY,
                     path);

          g_free (path);
        }
    }
  else if (response == EMPATHY_AVATAR_CHOOSER_RESPONSE_NO_IMAGE)
    {
      /* This corresponds to "No Image", not to "Cancel" */
      avatar_chooser_clear_image (self);
    }
  #ifdef HAVE_CHEESE
  else if (response == EMPATHY_AVATAR_CHOOSER_RESPONSE_WEBCAM)
    {
      /* This corresponds to "Camera Picture" */
      choose_avatar_from_webcam (widget, self);
    }
  #endif

  gtk_widget_destroy (widget);
}

static void
avatar_chooser_clicked_cb (GtkWidget *button,
    EmpathyAvatarChooser *self)
{
  GtkFileChooser *chooser_dialog;
  GtkWidget *image;
  gchar *saved_dir = NULL;
  const gchar *default_dir = DEFAULT_DIR;
  const gchar *pics_dir;
  GtkFileFilter *filter;
#ifdef HAVE_CHEESE
  GtkWidget *picture_button;
  EmpathyCameraMonitor *monitor;
#endif

  if (self->priv->chooser_dialog != NULL)
    {
      gtk_window_present (GTK_WINDOW (self->priv->chooser_dialog));
      return;
    }

  self->priv->chooser_dialog = GTK_FILE_CHOOSER (
      gtk_file_chooser_dialog_new (_("Select Your Avatar Image"),
        empathy_get_toplevel_window (GTK_WIDGET (self)),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        NULL, NULL));

#ifdef HAVE_CHEESE
  picture_button = gtk_dialog_add_button (
      GTK_DIALOG (self->priv->chooser_dialog),
      _("Take a picture..."), EMPATHY_AVATAR_CHOOSER_RESPONSE_WEBCAM);

  /* Button is sensitive only if there is one camera connected */
  monitor = empathy_camera_monitor_dup_singleton ();

  g_object_set_data_full (G_OBJECT (picture_button),
      "monitor", monitor, g_object_unref);

  g_object_bind_property (monitor, "available", picture_button, "sensitive",
      G_BINDING_SYNC_CREATE);
#endif

  gtk_dialog_add_buttons (GTK_DIALOG (self->priv->chooser_dialog),
      _("No Image"), EMPATHY_AVATAR_CHOOSER_RESPONSE_NO_IMAGE,
      GTK_STOCK_CANCEL, EMPATHY_AVATAR_CHOOSER_RESPONSE_CANCEL,
      GTK_STOCK_OPEN, EMPATHY_AVATAR_CHOOSER_RESPONSE_FILE,
      NULL);

  chooser_dialog = self->priv->chooser_dialog;
  gtk_window_set_destroy_with_parent (GTK_WINDOW (chooser_dialog), TRUE);

  /* Get special dirs */
  saved_dir = g_settings_get_string (self->priv->gsettings_ui,
             EMPATHY_PREFS_UI_AVATAR_DIRECTORY);

  if (saved_dir != NULL &&
      !g_file_test (saved_dir, G_FILE_TEST_IS_DIR))
    {
      g_free (saved_dir);
      saved_dir = NULL;
    }

  if (!g_file_test (default_dir, G_FILE_TEST_IS_DIR))
    default_dir = NULL;

  pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
  if (pics_dir != NULL && !g_file_test (pics_dir, G_FILE_TEST_IS_DIR))
    pics_dir = NULL;

  /* Set current dir to the last one or to DEFAULT_DIR or to home */
  if (saved_dir != NULL)
    gtk_file_chooser_set_current_folder (chooser_dialog, saved_dir);
  else if (pics_dir != NULL)
    gtk_file_chooser_set_current_folder (chooser_dialog, pics_dir);
  else if (default_dir != NULL)
    gtk_file_chooser_set_current_folder (chooser_dialog, default_dir);
  else
    gtk_file_chooser_set_current_folder (chooser_dialog, g_get_home_dir ());

  /* Add shortcuts to special dirs */
  if (saved_dir)
    gtk_file_chooser_add_shortcut_folder (chooser_dialog, saved_dir, NULL);
  else if (pics_dir)
    gtk_file_chooser_add_shortcut_folder (chooser_dialog, pics_dir, NULL);

  if (default_dir != NULL)
    gtk_file_chooser_add_shortcut_folder (chooser_dialog, default_dir, NULL);

  /* Setup preview image */
  image = gtk_image_new ();
  gtk_file_chooser_set_preview_widget (chooser_dialog, image);
  gtk_widget_set_size_request (image, AVATAR_SIZE_SAVE, AVATAR_SIZE_SAVE);
  gtk_widget_show (image);
  gtk_file_chooser_set_use_preview_label (chooser_dialog, FALSE);
  g_signal_connect (chooser_dialog, "update-preview",
      G_CALLBACK (avatar_chooser_update_preview_cb),
      self);

  /* Setup filers */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Images"));
  gtk_file_filter_add_pixbuf_formats (filter);
  gtk_file_chooser_add_filter (chooser_dialog, filter);
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All Files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (chooser_dialog, filter);

  /* Setup response */
  gtk_dialog_set_default_response (GTK_DIALOG (chooser_dialog),
      EMPATHY_AVATAR_CHOOSER_RESPONSE_FILE);

  g_signal_connect (chooser_dialog, "response",
      G_CALLBACK (avatar_chooser_response_cb),
      self);

  gtk_widget_show (GTK_WIDGET (chooser_dialog));

  g_free (saved_dir);
}

static void
empathy_avatar_chooser_init (EmpathyAvatarChooser *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
    EMPATHY_TYPE_AVATAR_CHOOSER, EmpathyAvatarChooserPrivate);

  gtk_drag_dest_set (GTK_WIDGET (self),
      GTK_DEST_DEFAULT_ALL,
      drop_types,
      G_N_ELEMENTS (drop_types),
      GDK_ACTION_COPY);

  self->priv->gsettings_ui = g_settings_new (EMPATHY_PREFS_UI_SCHEMA);

  g_signal_connect (self, "drag-motion",
      G_CALLBACK (avatar_chooser_drag_motion_cb),
      self);
  g_signal_connect (self, "drag-drop",
      G_CALLBACK (avatar_chooser_drag_drop_cb),
      self);
  g_signal_connect (self, "drag-data-received",
      G_CALLBACK (avatar_chooser_drag_data_received_cb),
      self);
  g_signal_connect (self, "clicked",
      G_CALLBACK (avatar_chooser_clicked_cb),
      self);

  empathy_avatar_chooser_set (self, NULL);
}

static void
avatar_chooser_set_image_from_avatar (EmpathyAvatarChooser *self,
    EmpathyAvatar *avatar,
    gboolean set_locally)
{
  GdkPixbuf *pixbuf;
  gchar *mime_type = NULL;

  g_assert (avatar != NULL);

  pixbuf = empathy_pixbuf_from_data_and_mime ((gchar *) avatar->data,
      avatar->len, &mime_type);

  if (pixbuf == NULL)
    {
      DEBUG ("couldn't make a pixbuf from avatar; giving up");
      return;
    }

  if (avatar->format == NULL)
    {
      avatar->format = mime_type;
    }
  else
    {
      if (strcmp (mime_type, avatar->format))
        DEBUG ("avatar->format is %s; gdkpixbuf yields %s!",
          avatar->format, mime_type);

      g_free (mime_type);
    }

  empathy_avatar_ref (avatar);

  avatar_chooser_set_image (self, avatar, pixbuf, set_locally);
}

/**
 * empathy_avatar_chooser_new:
 *
 * Creates a new #EmpathyAvatarChooser.
 *
 * Return value: a new #EmpathyAvatarChooser
 */
GtkWidget *
empathy_avatar_chooser_new (void)
{
  return g_object_new (EMPATHY_TYPE_AVATAR_CHOOSER, NULL);
}

/**
 * empathy_avatar_chooser_set:
 * @self: an #EmpathyAvatarChooser
 * @avatar: a new #EmpathyAvatar
 *
 * Sets the @self to display the avatar indicated by @avatar.
 */
void
empathy_avatar_chooser_set (EmpathyAvatarChooser *self,
    EmpathyAvatar *avatar)
{
  g_return_if_fail (EMPATHY_IS_AVATAR_CHOOSER (self));

  if (avatar != NULL)
    avatar_chooser_set_image_from_avatar (self, avatar, FALSE);
  else
    avatar_chooser_clear_image (self);
}

/**
 * empathy_avatar_chooser_get_image_data:
 * @self: an #EmpathyAvatarChooser
 * @data: avatar bytes
 * @data_size: size of @data
 * @mime_type: avatar mime-type
 *
 * Gets image data about the currently selected avatar.
 */
void
empathy_avatar_chooser_get_image_data (EmpathyAvatarChooser  *self,
    const gchar **data,
    gsize *data_size,
    const gchar **mime_type)
{
  g_return_if_fail (EMPATHY_IS_AVATAR_CHOOSER (self));

  if (self->priv->avatar != NULL)
    {
      if (data != NULL)
        *data = (gchar *) self->priv->avatar->data;

      if (data_size != NULL)
        *data_size = self->priv->avatar->len;

      if (mime_type != NULL)
        *mime_type = self->priv->avatar->format;
  }
  else
    {
      if (data != NULL)
        *data = NULL;

      if (data_size != NULL)
        *data_size = 0;

      if (mime_type != NULL)
        *mime_type = NULL;
    }
}

void
empathy_avatar_chooser_set_account (EmpathyAvatarChooser *self,
    TpAccount *account)
{
  g_return_if_fail (account != NULL);

  avatar_chooser_set_connection (self, tp_account_get_connection (account));
  g_object_notify (G_OBJECT (self), "connection");
}
