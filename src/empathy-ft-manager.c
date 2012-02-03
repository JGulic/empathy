/*
 * Copyright (C) 2003, 2004 Xan Lopez
 * Copyright (C) 2007 Marco Barisione <marco@barisione.org>
 * Copyright (C) 2008-2009 Collabora Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Authors: Xan Lopez
 *          Marco Barisione <marco@barisione.org>
 *          Jonny Lamb <jonny.lamb@collabora.co.uk>
 *          Xavier Claessens <xclaesse@gmail.com>
 *          Cosimo Cecchi <cosimo.cecchi@collabora.co.uk>
 */

/* The original file transfer manager code was copied from Epiphany */

#include "config.h"

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#define DEBUG_FLAG EMPATHY_DEBUG_FT
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-utils.h>

#include <libempathy-gtk/empathy-ui-utils.h>
#include <libempathy-gtk/empathy-geometry.h>
#include <libempathy-gtk/empathy-images.h>

#include "empathy-ft-manager.h"

enum
{
  COL_PERCENT,
  COL_ICON,
  COL_MESSAGE,
  COL_REMAINING,
  COL_FT_OBJECT
};

typedef struct {
  GtkTreeModel *model;
  GHashTable *ft_handler_to_row_ref;

  /* Widgets */
  GtkWidget *window;
  GtkWidget *treeview;
  GtkWidget *open_button;
  GtkWidget *abort_button;
  GtkWidget *clear_button;
} EmpathyFTManagerPriv;

enum
{
  RESPONSE_OPEN  = 1,
  RESPONSE_STOP  = 2,
  RESPONSE_CLEAR = 3,
  RESPONSE_CLOSE = 4
};

G_DEFINE_TYPE (EmpathyFTManager, empathy_ft_manager, G_TYPE_OBJECT);

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyFTManager)

static EmpathyFTManager *manager_singleton = NULL;

static void ft_handler_hashing_started_cb (EmpathyFTHandler *handler,
    EmpathyFTManager *manager);

static gchar *
ft_manager_format_interval (guint interval)
{
  gint hours, mins, secs;

  hours = interval / 3600;
  interval -= hours * 3600;
  mins = interval / 60;
  interval -= mins * 60;
  secs = interval;

  if (hours > 0)
    /* Translators: time left, when it is more than one hour */
    return g_strdup_printf (_("%u:%02u.%02u"), hours, mins, secs);
  else
    /* Translators: time left, when is is less than one hour */
    return g_strdup_printf (_("%02u.%02u"), mins, secs);
}

static void
ft_manager_update_buttons (EmpathyFTManager *manager)
{
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;
  EmpathyFTHandler *handler;
  gboolean open_enabled = FALSE;
  gboolean abort_enabled = FALSE;
  gboolean clear_enabled = FALSE;
  gboolean is_completed, is_cancelled;
  GHashTableIter hash_iter;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, COL_FT_OBJECT, &handler, -1);

      is_completed = empathy_ft_handler_is_completed (handler);
      is_cancelled = empathy_ft_handler_is_cancelled (handler);

      /* I can open the file if the transfer is completed and was incoming */
      open_enabled = (is_completed && empathy_ft_handler_is_incoming (handler));

      /* I can abort if the transfer is not already finished */
      abort_enabled = (is_cancelled == FALSE && is_completed == FALSE);

      g_object_unref (handler);
    }

  g_hash_table_iter_init (&hash_iter, priv->ft_handler_to_row_ref);

  while (g_hash_table_iter_next (&hash_iter, (gpointer *) &handler, NULL))
    {
      if (empathy_ft_handler_is_completed (handler) ||
          empathy_ft_handler_is_cancelled (handler))
          clear_enabled = TRUE;

      if (clear_enabled)
        break;
    }

  gtk_widget_set_sensitive (priv->open_button, open_enabled);
  gtk_widget_set_sensitive (priv->abort_button, abort_enabled);

  if (clear_enabled)
    gtk_widget_set_sensitive (priv->clear_button, TRUE);
}

static void
ft_manager_selection_changed (GtkTreeSelection *selection,
                              EmpathyFTManager *manager)
{
  ft_manager_update_buttons (manager);
}

static void
ft_manager_progress_cell_data_func (GtkTreeViewColumn *col,
                                    GtkCellRenderer *renderer,
                                    GtkTreeModel *model,
                                    GtkTreeIter *iter,
                                    gpointer user_data)
{
  const gchar *text = NULL;
  gint percent;

  gtk_tree_model_get (model, iter, COL_PERCENT, &percent, -1);

  if (percent < 0)
    {
      percent = 0;
      text = C_("file transfer percent", "Unknown");
    }

  g_object_set (renderer, "text", text, "value", percent, NULL);
}

static GtkTreeRowReference *
ft_manager_get_row_from_handler (EmpathyFTManager *manager,
                                 EmpathyFTHandler *handler)
{
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  return g_hash_table_lookup (priv->ft_handler_to_row_ref, handler);
}

static void
ft_manager_remove_file_from_model (EmpathyFTManager *manager,
                                   EmpathyFTHandler *handler)
{
  GtkTreeRowReference *row_ref;
  GtkTreeSelection *selection;
  GtkTreePath *path = NULL;
  GtkTreeIter iter;
  gboolean update_selection;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref);

  DEBUG ("Removing file transfer from window: contact=%s, filename=%s",
      empathy_contact_get_alias (empathy_ft_handler_get_contact (handler)),
      empathy_ft_handler_get_filename (handler));

  /* Get the iter from the row_ref */
  path = gtk_tree_row_reference_get_path (row_ref);
  gtk_tree_model_get_iter (priv->model, &iter, path);
  gtk_tree_path_free (path);

  /* We have to update the selection only if we are removing the selected row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  update_selection = gtk_tree_selection_iter_is_selected (selection, &iter);

  /* Remove tp_file's row. After that iter points to the next row */
  if (!gtk_list_store_remove (GTK_LIST_STORE (priv->model), &iter))
    {
      gint n_row;

      /* There is no next row, set iter to the last row */
      n_row = gtk_tree_model_iter_n_children (priv->model, NULL);
      if (n_row > 0)
        gtk_tree_model_iter_nth_child (priv->model, &iter, NULL, n_row - 1);
      else
        update_selection = FALSE;
    }

  if (update_selection)
    gtk_tree_selection_select_iter (selection, &iter);
}

static gboolean
remove_finished_transfer_foreach (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
  EmpathyFTHandler *handler = key;
  EmpathyFTManager *manager = user_data;

  if (empathy_ft_handler_is_completed (handler) ||
      empathy_ft_handler_is_cancelled (handler))
    {
      ft_manager_remove_file_from_model (manager, handler);
      return TRUE;
    }

  return FALSE;
}

static gchar *
ft_manager_format_progress_bytes_and_percentage (guint64 current,
                                                 guint64 total,
                                                 gdouble speed,
                                                 int *percentage)
{
  char *total_str, *current_str, *retval;
  char *speed_str = NULL;

  total_str = g_format_size (total);
  current_str = g_format_size (current);

  if (speed > 0)
    speed_str = g_format_size ((goffset) speed);

  /* translators: first %s is the currently processed size, second %s is
   * the total file size */
  retval = speed_str ?
    g_strdup_printf (_("%s of %s at %s/s"), current_str, total_str, speed_str) :
    g_strdup_printf (_("%s of %s"), current_str, total_str);

  g_free (total_str);
  g_free (current_str);
  g_free (speed_str);

  if (percentage != NULL)
    {
      if (total != 0)
        *percentage = current * 100 / total;
      else
        *percentage = -1;
    }

  return retval;
}

static gchar *
ft_manager_format_contact_info (EmpathyFTHandler *handler)
{
  gboolean incoming;
  const char *filename, *contact_name, *first_line_format;
  char *retval;

  incoming = empathy_ft_handler_is_incoming (handler);
  contact_name = empathy_contact_get_alias
    (empathy_ft_handler_get_contact (handler));
  filename = empathy_ft_handler_get_filename (handler);

  if (incoming)
    /* translators: first %s is filename, second %s is the contact name */
    first_line_format = _("Receiving \"%s\" from %s");
  else
    /* translators: first %s is filename, second %s is the contact name */
    first_line_format = _("Sending \"%s\" to %s");

  retval = g_strdup_printf (first_line_format, filename, contact_name);

  return retval;
}

static gchar *
ft_manager_format_error_message (EmpathyFTHandler *handler,
                                 const GError *error)
{
  const char *contact_name, *filename;
  EmpathyContact *contact;
  char *first_line, *message;
  gboolean incoming;

  contact_name = NULL;
  incoming = empathy_ft_handler_is_incoming (handler);

  contact = empathy_ft_handler_get_contact (handler);
  if (contact)
    contact_name = empathy_contact_get_alias (contact);

  filename = empathy_ft_handler_get_filename (handler);

  if (incoming)
    /* filename/contact_name here are either both NULL or both valid */
    if (filename && contact_name)
      /* translators: first %s is filename, second %s
       * is the contact name */
      first_line = g_strdup_printf (_("Error receiving \"%s\" from %s"), filename,
          contact_name);
    else
      first_line = g_strdup (_("Error receiving a file"));
  else
    /* translators: first %s is filename, second %s
     * is the contact name */
    if (filename && contact_name)
      first_line = g_strdup_printf (_("Error sending \"%s\" to %s"), filename,
          contact_name);
    else
      first_line = g_strdup (_("Error sending a file"));

  message = g_strdup_printf ("%s\n%s", first_line, error->message);

  g_free (first_line);

  return message;
}

static void
ft_manager_update_handler_message (EmpathyFTManager *manager,
                                   GtkTreeRowReference *row_ref,
                                   const char *message)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  /* Set new value in the store */
  path = gtk_tree_row_reference_get_path (row_ref);
  gtk_tree_model_get_iter (priv->model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (priv->model),
      &iter,
      COL_MESSAGE, message ? message : "",
      -1);

  gtk_tree_path_free (path);
}

static void
ft_manager_update_handler_progress (EmpathyFTManager *manager,
                                    GtkTreeRowReference *row_ref,
                                    int percentage)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  /* Set new value in the store */
  path = gtk_tree_row_reference_get_path (row_ref);
  gtk_tree_model_get_iter (priv->model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (priv->model),
      &iter,
      COL_PERCENT, percentage,
      -1);

  gtk_tree_path_free (path);

}

static void
ft_manager_update_handler_time (EmpathyFTManager *manager,
                                GtkTreeRowReference *row_ref,
                                guint remaining_time)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);
  char *remaining_str;

  remaining_str = ft_manager_format_interval (remaining_time);

  /* Set new value in the store */
  path = gtk_tree_row_reference_get_path (row_ref);
  gtk_tree_model_get_iter (priv->model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (priv->model),
      &iter,
      COL_REMAINING, remaining_str,
      -1);

  gtk_tree_path_free (path);
  g_free (remaining_str);
}

static void
ft_manager_clear_handler_time (EmpathyFTManager *manager,
                               GtkTreeRowReference *row_ref)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  /* Set new value in the store */
  path = gtk_tree_row_reference_get_path (row_ref);
  gtk_tree_model_get_iter (priv->model, &iter, path);
  gtk_list_store_set (GTK_LIST_STORE (priv->model),
      &iter,
      COL_REMAINING, NULL,
      -1);

  gtk_tree_path_free (path);
}

static void
ft_handler_transfer_error_cb (EmpathyFTHandler *handler,
                              GError *error,
                              EmpathyFTManager *manager)
{
  char *message;
  GtkTreeRowReference *row_ref;

  DEBUG ("Transfer error %s", error->message);

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  message = ft_manager_format_error_message (handler, error);

  ft_manager_update_handler_message (manager, row_ref, message);
  ft_manager_clear_handler_time (manager, row_ref);
  ft_manager_update_buttons (manager);

  g_free (message);
}

static void
do_real_transfer_done (EmpathyFTManager *manager,
                       EmpathyFTHandler *handler)
{
  const char *contact_name;
  const char *filename;
  char *first_line, *second_line, *message;
  char *uri;
  gboolean incoming;
  GtkTreeRowReference *row_ref;
  GtkRecentManager *recent_manager;
  GFile *file;

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  incoming = empathy_ft_handler_is_incoming (handler);
  contact_name = empathy_contact_get_alias
    (empathy_ft_handler_get_contact (handler));
  filename = empathy_ft_handler_get_filename (handler);

  if (incoming)
    /* translators: first %s is filename, second %s
     * is the contact name */
    first_line = g_strdup_printf (_("\"%s\" received from %s"), filename,
        contact_name);
  else
    /* translators: first %s is filename, second %s
     * is the contact name */
    first_line = g_strdup_printf (_("\"%s\" sent to %s"), filename,
        contact_name);

  second_line = g_strdup (_("File transfer completed"));

  message = g_strdup_printf ("%s\n%s", first_line, second_line);
  ft_manager_update_handler_message (manager, row_ref, message);
  ft_manager_clear_handler_time (manager, row_ref);

  /* update buttons */
  ft_manager_update_buttons (manager);

  g_free (message);
  g_free (first_line);
  g_free (second_line);

  recent_manager = gtk_recent_manager_get_default ();
  file = empathy_ft_handler_get_gfile (handler);
  uri = g_file_get_uri (file);

  gtk_recent_manager_add_item (recent_manager, uri);

  g_free (uri);
}

static void
ft_handler_transfer_done_cb (EmpathyFTHandler *handler,
                             TpFileTransferChannel *channel,
                             EmpathyFTManager *manager)
{
  if (empathy_ft_handler_is_incoming (handler) &&
      empathy_ft_handler_get_use_hash (handler))
    {
      DEBUG ("Transfer done, waiting for hashing-started");

      /* connect to the signal and return early */
      g_signal_connect (handler, "hashing-started",
          G_CALLBACK (ft_handler_hashing_started_cb), manager);

      return;
    }

  DEBUG ("Transfer done, no hashing");

  do_real_transfer_done (manager, handler);
}

static void
ft_handler_transfer_progress_cb (EmpathyFTHandler *handler,
                                 guint64 current_bytes,
                                 guint64 total_bytes,
                                 guint remaining_time,
                                 gdouble speed,
                                 EmpathyFTManager *manager)
{
  char *first_line, *second_line, *message;
  int percentage;
  GtkTreeRowReference *row_ref;

  DEBUG ("Transfer progress");

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  first_line = ft_manager_format_contact_info (handler);
  second_line = ft_manager_format_progress_bytes_and_percentage
    (current_bytes, total_bytes, speed, &percentage);

  message = g_strdup_printf ("%s\n%s", first_line, second_line);

  ft_manager_update_handler_message (manager, row_ref, message);
  ft_manager_update_handler_progress (manager, row_ref, percentage);

  if (remaining_time > 0)
    ft_manager_update_handler_time (manager, row_ref, remaining_time);

  g_free (message);
  g_free (first_line);
  g_free (second_line);
}

static void
ft_handler_transfer_started_cb (EmpathyFTHandler *handler,
                                TpFileTransferChannel *channel,
                                EmpathyFTManager *manager)
{
  guint64 transferred_bytes, total_bytes;

  DEBUG ("Transfer started");

  g_signal_connect (handler, "transfer-progress",
      G_CALLBACK (ft_handler_transfer_progress_cb), manager);
  g_signal_connect (handler, "transfer-done",
      G_CALLBACK (ft_handler_transfer_done_cb), manager);

  transferred_bytes = empathy_ft_handler_get_transferred_bytes (handler);
  total_bytes = empathy_ft_handler_get_total_bytes (handler);

  ft_handler_transfer_progress_cb (handler, transferred_bytes, total_bytes,
      0, -1, manager);
}

static void
ft_handler_hashing_done_cb (EmpathyFTHandler *handler,
                            EmpathyFTManager *manager)
{
  GtkTreeRowReference *row_ref;
  char *first_line, *second_line, *message;

  DEBUG ("Hashing done");

  /* update the message */
  if (empathy_ft_handler_is_incoming (handler))
    {
      do_real_transfer_done (manager, handler);
      return;
    }

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  first_line = ft_manager_format_contact_info (handler);
  second_line = g_strdup (_("Waiting for the other participant's response"));
  message = g_strdup_printf ("%s\n%s", first_line, second_line);

  ft_manager_update_handler_message (manager, row_ref, message);

  g_free (message);
  g_free (first_line);
  g_free (second_line);

  g_signal_connect (handler, "transfer-started",
      G_CALLBACK (ft_handler_transfer_started_cb), manager);
}

static void
ft_handler_hashing_progress_cb (EmpathyFTHandler *handler,
                                guint64 current_bytes,
                                guint64 total_bytes,
                                EmpathyFTManager *manager)
{
  char *first_line, *second_line, *message;
  GtkTreeRowReference *row_ref;

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  if (empathy_ft_handler_is_incoming (handler))
      first_line = g_strdup_printf (_("Checking integrity of \"%s\""),
          empathy_ft_handler_get_filename (handler));
  else
      first_line =  g_strdup_printf (_("Hashing \"%s\""),
          empathy_ft_handler_get_filename (handler));

  second_line = ft_manager_format_progress_bytes_and_percentage
    (current_bytes, total_bytes, -1, NULL);

  message = g_strdup_printf ("%s\n%s", first_line, second_line);

  ft_manager_update_handler_message (manager, row_ref, message);

  g_free (message);
  g_free (first_line);
  g_free (second_line);
}

static void
ft_handler_hashing_started_cb (EmpathyFTHandler *handler,
                               EmpathyFTManager *manager)
{
  char *message, *first_line, *second_line;
  GtkTreeRowReference *row_ref;

  DEBUG ("Hashing started");

  g_signal_connect (handler, "hashing-progress",
     G_CALLBACK (ft_handler_hashing_progress_cb), manager);
  g_signal_connect (handler, "hashing-done",
     G_CALLBACK (ft_handler_hashing_done_cb), manager);

  row_ref = ft_manager_get_row_from_handler (manager, handler);
  g_return_if_fail (row_ref != NULL);

  first_line = ft_manager_format_contact_info (handler);

  if (empathy_ft_handler_is_incoming (handler))
      second_line = g_strdup_printf (_("Checking integrity of \"%s\""),
          empathy_ft_handler_get_filename (handler));
  else
      second_line = g_strdup_printf (_("Hashing \"%s\""),
          empathy_ft_handler_get_filename (handler));

  message = g_strdup_printf ("%s\n%s", first_line, second_line);

  ft_manager_update_handler_message (manager, row_ref, message);

  g_free (first_line);
  g_free (second_line);
  g_free (message);
}

static void
ft_manager_start_transfer (EmpathyFTManager *manager,
                           EmpathyFTHandler *handler)
{
  gboolean is_outgoing;

  is_outgoing = !empathy_ft_handler_is_incoming (handler);

  DEBUG ("Start transfer, is outgoing %s",
      is_outgoing ? "True" : "False");

  /* now connect the signals */
  g_signal_connect (handler, "transfer-error",
      G_CALLBACK (ft_handler_transfer_error_cb), manager);

  if (is_outgoing && empathy_ft_handler_get_use_hash (handler)) {
    g_signal_connect (handler, "hashing-started",
        G_CALLBACK (ft_handler_hashing_started_cb), manager);
  } else {
    /* either incoming or outgoing without hash */
    g_signal_connect (handler, "transfer-started",
        G_CALLBACK (ft_handler_transfer_started_cb), manager);
  }

  empathy_ft_handler_start_transfer (handler);
}

static void
ft_manager_add_handler_to_list (EmpathyFTManager *manager,
                                EmpathyFTHandler *handler,
                                const GError *error)
{
  GtkTreeRowReference *row_ref;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreePath *path;
  GIcon *icon;
  const char *content_type, *second_line;
  char *first_line, *message;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  icon = NULL;

  /* get the icon name from the mime-type of the file. */
  content_type = empathy_ft_handler_get_content_type (handler);

  if (content_type != NULL)
    icon = g_content_type_get_icon (content_type);

  /* append the handler in the store */
  gtk_list_store_insert_with_values (GTK_LIST_STORE (priv->model),
      &iter, G_MAXINT, COL_FT_OBJECT, handler,
      COL_ICON, icon, -1);

  if (icon != NULL)
    g_object_unref (icon);

  /* insert the new row_ref in the hash table  */
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (priv->model), &iter);
  row_ref = gtk_tree_row_reference_new (GTK_TREE_MODEL (priv->model), path);
  gtk_tree_path_free (path);
  g_hash_table_insert (priv->ft_handler_to_row_ref, g_object_ref (handler),
      row_ref);

  /* select the new row */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
  gtk_tree_selection_select_iter (selection, &iter);

  if (error != NULL)
    {
      message = ft_manager_format_error_message (handler, error);
      ft_manager_update_handler_message (manager, row_ref, message);

      g_free (message);
      return;
    }

  /* update the row with the initial values.
   * the only case where we postpone this is in case we're managing
   * an outgoing+hashing transfer, as the hashing started signal will
   * take care of updating the information.
   */
  if (empathy_ft_handler_is_incoming (handler) ||
      !empathy_ft_handler_get_use_hash (handler)) {
    first_line = ft_manager_format_contact_info (handler);
    second_line = _("Waiting for the other participant's response");
    message = g_strdup_printf ("%s\n%s", first_line, second_line);

    ft_manager_update_handler_message (manager, row_ref, message);

    g_free (first_line);
    g_free (message);
  }

  /* hook up the signals and start the transfer */
  ft_manager_start_transfer (manager, handler);
}

static void
ft_manager_clear (EmpathyFTManager *manager)
{
  EmpathyFTManagerPriv *priv;

  DEBUG ("Clearing file transfer list");

  priv = GET_PRIV (manager);

  /* Remove completed and cancelled transfers */
  g_hash_table_foreach_remove (priv->ft_handler_to_row_ref,
      remove_finished_transfer_foreach, manager);

  /* set the clear button back to insensitive */
  gtk_widget_set_sensitive (priv->clear_button, FALSE);
}

static void
ft_manager_open (EmpathyFTManager *manager)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyFTHandler *handler;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, COL_FT_OBJECT, &handler, -1);

  if (empathy_ft_handler_is_completed (handler)){
    char *uri;
    GFile *file;

    file = empathy_ft_handler_get_gfile (handler);
    uri = g_file_get_uri (file);

    DEBUG ("Opening URI: %s", uri);
    empathy_url_show (GTK_WIDGET (priv->window), uri);
    g_free (uri);
  }

  g_object_unref (handler);
}

static void
ft_manager_stop (EmpathyFTManager *manager)
{
  GtkTreeSelection *selection;
  GtkTreeIter iter;
  GtkTreeModel *model;
  EmpathyFTHandler *handler;
  EmpathyFTManagerPriv *priv;

  priv = GET_PRIV (manager);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter, COL_FT_OBJECT, &handler, -1);
  g_return_if_fail (handler != NULL);

  DEBUG ("Stopping file transfer: contact=%s, filename=%s",
      empathy_contact_get_alias (empathy_ft_handler_get_contact (handler)),
      empathy_ft_handler_get_filename (handler));

  empathy_ft_handler_cancel_transfer (handler);

  g_object_unref (handler);
}

static gboolean
close_window (EmpathyFTManager *manager)
{
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  DEBUG ("%p", manager);

  /* remove all the completed/cancelled/errored transfers */
  ft_manager_clear (manager);

  if (g_hash_table_size (priv->ft_handler_to_row_ref) > 0)
    {
      /* There is still FTs on flight, just hide the window */
      DEBUG ("Hiding window");
      gtk_widget_hide (priv->window);
      return TRUE;
    }

  return FALSE;
}

static void
ft_manager_response_cb (GtkWidget *widget,
                        gint response,
                        EmpathyFTManager *manager)
{
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  switch (response)
    {
      case RESPONSE_CLEAR:
        ft_manager_clear (manager);
        break;
      case RESPONSE_OPEN:
        ft_manager_open (manager);
        break;
      case RESPONSE_STOP:
        ft_manager_stop (manager);
        break;
      case RESPONSE_CLOSE:
        if (!close_window (manager))
          gtk_widget_destroy (priv->window);
        break;
      case GTK_RESPONSE_NONE:
      case GTK_RESPONSE_DELETE_EVENT:
        /* Do nothing */
        break;
      default:
        g_assert_not_reached ();
    }
}

static gboolean
ft_manager_delete_event_cb (GtkWidget *widget,
                            GdkEvent *event,
                            EmpathyFTManager *manager)
{
  return close_window (manager);
}

static void
ft_manager_destroy_cb (GtkWidget *widget,
                       EmpathyFTManager *manager)
{
  DEBUG ("%p", manager);

  g_object_unref (manager);
}

static gboolean
ft_view_button_press_event_cb (GtkWidget *widget,
                               GdkEventKey *event,
                               EmpathyFTManager *manager)
{

  if (event->type != GDK_2BUTTON_PRESS)
      return FALSE;

  ft_manager_open (manager);

  return FALSE;
}

static gboolean
ft_manager_key_press_event_cb (GtkWidget *widget,
                               GdkEventKey *event,
                               gpointer user_data)
{
  if ((event->state & GDK_CONTROL_MASK && event->keyval == GDK_KEY_w)
      || event->keyval == GDK_KEY_Escape)
    {
      gtk_widget_destroy (widget);
      return TRUE;
    }

  return FALSE;
}

static void
ft_manager_build_ui (EmpathyFTManager *manager)
{
  GtkBuilder *gui;
  GtkTreeView *view;
  GtkListStore *liststore;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  gchar *filename;
  EmpathyFTManagerPriv *priv = GET_PRIV (manager);

  filename = empathy_file_lookup ("empathy-ft-manager.ui", "src");
  gui = empathy_builder_get_file (filename,
      "ft_manager_dialog", &priv->window,
      "ft_list", &priv->treeview,
      "clear_button", &priv->clear_button,
      "open_button", &priv->open_button,
      "abort_button", &priv->abort_button,
      NULL);
  g_free (filename);

  empathy_builder_connect (gui, manager,
      "ft_manager_dialog", "destroy", ft_manager_destroy_cb,
      "ft_manager_dialog", "response", ft_manager_response_cb,
      "ft_manager_dialog", "delete-event", ft_manager_delete_event_cb,
      "ft_manager_dialog", "key-press-event", ft_manager_key_press_event_cb,
      NULL);

  empathy_builder_unref_and_keep_widget (gui, priv->window);

  /* Window geometry. */
  empathy_geometry_bind (GTK_WINDOW (priv->window), "ft-manager");

  /* Setup the tree view */
  view = GTK_TREE_VIEW (priv->treeview);
  selection = gtk_tree_view_get_selection (view);
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_BROWSE);
  g_signal_connect (selection, "changed",
      G_CALLBACK (ft_manager_selection_changed), manager);
  g_signal_connect (view, "button-press-event",
                    G_CALLBACK (ft_view_button_press_event_cb),
                    manager);
  gtk_tree_view_set_headers_visible (view, TRUE);
  gtk_tree_view_set_enable_search (view, FALSE);

  /* Setup the model */
  liststore = gtk_list_store_new (5,
      G_TYPE_INT,     /* percent */
      G_TYPE_ICON,    /* icon */
      G_TYPE_STRING,  /* message */
      G_TYPE_STRING,  /* remaining */
      G_TYPE_OBJECT); /* ft_handler */
  gtk_tree_view_set_model (view, GTK_TREE_MODEL (liststore));
  priv->model = GTK_TREE_MODEL (liststore);
  g_object_unref (liststore);

  /* Progress column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("%"));
  gtk_tree_view_column_set_sort_column_id (column, COL_PERCENT);
  gtk_tree_view_insert_column (view, column, -1);

  renderer = gtk_cell_renderer_progress_new ();
  g_object_set (renderer, "xalign", 0.5, NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
      ft_manager_progress_cell_data_func, NULL, NULL);

  /* Icon and filename column*/
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("File"));
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_sort_column_id (column, COL_MESSAGE);
  gtk_tree_view_column_set_spacing (column, 3);
  gtk_tree_view_insert_column (view, column, -1);

  renderer = gtk_cell_renderer_pixbuf_new ();
  g_object_set (renderer, "xpad", 3,
      "stock-size", GTK_ICON_SIZE_DND, NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
      "gicon", COL_ICON, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_attributes (column, renderer,
      "text", COL_MESSAGE, NULL);

  /* Remaining time column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Remaining"));
  gtk_tree_view_column_set_sort_column_id (column, COL_REMAINING);
  gtk_tree_view_insert_column (view, column, -1);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.5, NULL);
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_attributes (column, renderer,
      "text", COL_REMAINING, NULL);

  /* clear button should be sensitive only if there are completed/cancelled
   * handlers in the store.
   */
  gtk_widget_set_sensitive (priv->clear_button, FALSE);
}

/* GObject method overrides */

static void
empathy_ft_manager_finalize (GObject *object)
{
  EmpathyFTManagerPriv *priv = GET_PRIV (object);

  DEBUG ("FT Manager %p", object);

  g_hash_table_unref (priv->ft_handler_to_row_ref);

  G_OBJECT_CLASS (empathy_ft_manager_parent_class)->finalize (object);
}

static void
empathy_ft_manager_init (EmpathyFTManager *manager)
{
  EmpathyFTManagerPriv *priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE ((manager), EMPATHY_TYPE_FT_MANAGER,
      EmpathyFTManagerPriv);

  manager->priv = priv;

  priv->ft_handler_to_row_ref = g_hash_table_new_full (g_direct_hash,
      g_direct_equal, (GDestroyNotify) g_object_unref,
      (GDestroyNotify) gtk_tree_row_reference_free);

  ft_manager_build_ui (manager);
}

static GObject *
empathy_ft_manager_constructor (GType type,
                                guint n_props,
                                GObjectConstructParam *props)
{
  GObject *retval;

  if (manager_singleton)
    {
      retval = G_OBJECT (manager_singleton);
    }
  else
    {
      retval = G_OBJECT_CLASS (empathy_ft_manager_parent_class)->constructor
          (type, n_props, props);

      manager_singleton = EMPATHY_FT_MANAGER (retval);
      g_object_add_weak_pointer (retval, (gpointer) &manager_singleton);
    }

  return retval;
}

static void
empathy_ft_manager_class_init (EmpathyFTManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = empathy_ft_manager_finalize;
  object_class->constructor = empathy_ft_manager_constructor;

  g_type_class_add_private (object_class, sizeof (EmpathyFTManagerPriv));
}

/* public methods */

void
empathy_ft_manager_add_handler (EmpathyFTHandler *handler)
{
  EmpathyFTManager *manager;
  EmpathyFTManagerPriv *priv;

  DEBUG ("Adding handler");

  g_return_if_fail (EMPATHY_IS_FT_HANDLER (handler));

  manager = g_object_new (EMPATHY_TYPE_FT_MANAGER, NULL);
  priv = GET_PRIV (manager);

  ft_manager_add_handler_to_list (manager, handler, NULL);
  gtk_window_present (GTK_WINDOW (priv->window));
}

void
empathy_ft_manager_display_error (EmpathyFTHandler *handler,
                                  const GError *error)
{
  EmpathyFTManager *manager;
  EmpathyFTManagerPriv *priv;

  g_return_if_fail (EMPATHY_IS_FT_HANDLER (handler));
  g_return_if_fail (error != NULL);

  manager = g_object_new (EMPATHY_TYPE_FT_MANAGER, NULL);
  priv = GET_PRIV (manager);

  ft_manager_add_handler_to_list (manager, handler, error);
  gtk_window_present (GTK_WINDOW (priv->window));
}

void
empathy_ft_manager_show (void)
{
  EmpathyFTManager *manager;
  EmpathyFTManagerPriv *priv;

  manager = g_object_new (EMPATHY_TYPE_FT_MANAGER, NULL);
  priv = GET_PRIV (manager);

  gtk_window_present (GTK_WINDOW (priv->window));
}
