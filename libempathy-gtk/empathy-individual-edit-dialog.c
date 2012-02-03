/*
 * Copyright (C) 2007-2010 Collabora Ltd.
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
 *
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 *          Philip Withnall <philip.withnall@collabora.co.uk>
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

#include <telepathy-glib/util.h>
#include <folks/folks.h>

#include <libempathy/empathy-individual-manager.h>
#include <libempathy/empathy-utils.h>

#include "empathy-individual-edit-dialog.h"
#include "empathy-individual-widget.h"
#include "empathy-ui-utils.h"

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyIndividualEditDialog)

typedef struct {
  FolksIndividual *individual; /* owned */
  GtkWidget *individual_widget; /* child widget */
} EmpathyIndividualEditDialogPriv;

enum {
  PROP_INDIVIDUAL = 1,
};

/* Edit dialogs currently open.
 * Each dialog contains a referenced pointer to its Individual */
static GList *edit_dialogs = NULL;

static void individual_edit_dialog_set_individual (
    EmpathyIndividualEditDialog *dialog,
    FolksIndividual *individual);

G_DEFINE_TYPE (EmpathyIndividualEditDialog, empathy_individual_edit_dialog,
    GTK_TYPE_DIALOG);

static void
individual_dialogs_response_cb (GtkDialog *dialog,
    gint response,
    GList **dialogs)
{
  *dialogs = g_list_remove (*dialogs, dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}

static gint
individual_dialogs_find (GObject *object,
    FolksIndividual *individual)
{
  EmpathyIndividualEditDialogPriv *priv = GET_PRIV (object);

  return individual != priv->individual;
}

void
empathy_individual_edit_dialog_show (FolksIndividual *individual,
    GtkWindow *parent)
{
  GtkWidget *dialog;
  GList *l;

  g_return_if_fail (FOLKS_IS_INDIVIDUAL (individual));
  g_return_if_fail (parent == NULL || GTK_IS_WINDOW (parent));

  l = g_list_find_custom (edit_dialogs, individual,
      (GCompareFunc) individual_dialogs_find);

  if (l != NULL)
    {
      gtk_window_present (GTK_WINDOW (l->data));
      return;
    }

  /* Create dialog */
  dialog = g_object_new (EMPATHY_TYPE_INDIVIDUAL_EDIT_DIALOG,
      "individual", individual,
      NULL);

  edit_dialogs = g_list_prepend (edit_dialogs, dialog);
  gtk_widget_show (dialog);
}

static void
individual_removed_cb (FolksIndividual *individual,
    FolksIndividual *replacement_individual,
    EmpathyIndividualEditDialog *self)
{
  /* Update to show the new Individual (this will close the dialogue if there
   * is no new Individual). */
  individual_edit_dialog_set_individual (self, replacement_individual);

  /* Destroy the dialogue */
  if (replacement_individual == NULL)
    {
      individual_dialogs_response_cb (GTK_DIALOG (self),
          GTK_RESPONSE_DELETE_EVENT, &edit_dialogs);
    }
}

static void
individual_edit_dialog_set_individual (
    EmpathyIndividualEditDialog *dialog,
    FolksIndividual *individual)
{
  EmpathyIndividualEditDialogPriv *priv;

  g_return_if_fail (EMPATHY_INDIVIDUAL_EDIT_DIALOG (dialog));
  g_return_if_fail (individual == NULL || FOLKS_IS_INDIVIDUAL (individual));

  priv = GET_PRIV (dialog);

  /* Remove the old Individual */
  if (priv->individual != NULL)
    {
      g_signal_handlers_disconnect_by_func (priv->individual,
          (GCallback) individual_removed_cb, dialog);
    }

  tp_clear_object (&priv->individual);

  /* Add the new Individual */
  priv->individual = individual;

  if (individual != NULL)
    {
      g_object_ref (individual);
      g_signal_connect (individual, "removed",
          (GCallback) individual_removed_cb, dialog);

      /* Update the UI */
      empathy_individual_widget_set_individual (
          EMPATHY_INDIVIDUAL_WIDGET (priv->individual_widget), individual);
    }
}

static void
individual_edit_dialog_get_property (GObject *object,
    guint param_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyIndividualEditDialogPriv *priv = GET_PRIV (object);

  switch (param_id) {
  case PROP_INDIVIDUAL:
    g_value_set_object (value, priv->individual);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  };
}

static void
individual_edit_dialog_set_property (GObject *object,
  guint param_id,
  const GValue *value,
  GParamSpec   *pspec)
{
  EmpathyIndividualEditDialog *dialog =
    EMPATHY_INDIVIDUAL_EDIT_DIALOG (object);

  switch (param_id) {
  case PROP_INDIVIDUAL:
    individual_edit_dialog_set_individual (dialog,
        FOLKS_INDIVIDUAL (g_value_get_object (value)));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
    break;
  };
}

static void
individual_edit_dialog_dispose (GObject *object)
{
  individual_edit_dialog_set_individual (
      EMPATHY_INDIVIDUAL_EDIT_DIALOG (object), NULL);

  G_OBJECT_CLASS (
      empathy_individual_edit_dialog_parent_class)->dispose (object);
}

static void
empathy_individual_edit_dialog_class_init (
    EmpathyIndividualEditDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = individual_edit_dialog_get_property;
  object_class->set_property = individual_edit_dialog_set_property;
  object_class->dispose = individual_edit_dialog_dispose;

  g_object_class_install_property (object_class,
      PROP_INDIVIDUAL,
      g_param_spec_object ("individual",
          "Folks Individual",
          "Folks Individual to edit using the dialog.",
          FOLKS_TYPE_INDIVIDUAL,
          G_PARAM_CONSTRUCT_ONLY |
          G_PARAM_READWRITE |
          G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class,
      sizeof (EmpathyIndividualEditDialogPriv));
}

static void
empathy_individual_edit_dialog_init (
    EmpathyIndividualEditDialog *dialog)
{
  GtkWidget *button;
  EmpathyIndividualEditDialogPriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (
      dialog, EMPATHY_TYPE_INDIVIDUAL_EDIT_DIALOG,
      EmpathyIndividualEditDialogPriv);

  dialog->priv = priv;
  priv->individual = NULL;

  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_title (GTK_WINDOW (dialog), _("Edit Contact Information"));

  /* Individual widget */
  priv->individual_widget = empathy_individual_widget_new (priv->individual,
      EMPATHY_INDIVIDUAL_WIDGET_EDIT_ALIAS |
      EMPATHY_INDIVIDUAL_WIDGET_EDIT_GROUPS |
      EMPATHY_INDIVIDUAL_WIDGET_EDIT_FAVOURITE);
  gtk_container_set_border_width (GTK_CONTAINER (priv->individual_widget), 8);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (
      GTK_DIALOG (dialog))), priv->individual_widget, TRUE, TRUE, 0);
  gtk_widget_show (priv->individual_widget);

  /* Close button */
  button = gtk_button_new_with_label (GTK_STOCK_CLOSE);
  gtk_button_set_use_stock (GTK_BUTTON (button), TRUE);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
      GTK_RESPONSE_CLOSE);
  gtk_widget_set_can_default (button, TRUE);
  gtk_window_set_default (GTK_WINDOW (dialog), button);
  gtk_widget_show (button);

  g_signal_connect (dialog, "response",
      G_CALLBACK (individual_dialogs_response_cb), &edit_dialogs);
}
