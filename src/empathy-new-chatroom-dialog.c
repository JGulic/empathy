/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006-2007 Imendio AB
 * Copyright (C) 2007-2008 Collabora Ltd.
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
 * Authors: Martyn Russell <martyn@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#include <config.h>

#include <string.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>

#include <telepathy-glib/interfaces.h>

#include <libempathy/empathy-tp-roomlist.h>
#include <libempathy/empathy-chatroom.h>
#include <libempathy/empathy-utils.h>
#include <libempathy/empathy-request-util.h>
#include <libempathy/empathy-gsettings.h>

#include <libempathy-gtk/empathy-account-chooser.h>
#include <libempathy-gtk/empathy-ui-utils.h>

#include "empathy-new-chatroom-dialog.h"

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>

typedef struct {
	EmpathyTpRoomlist *room_list;
	/* Currently selected account */
	TpAccount         *account;
	/* Signal id of the "status-changed" signal connected on the currently
	 * selected account */
	gulong             status_changed_id;

	GtkWidget         *window;
	GtkWidget         *vbox_widgets;
	GtkWidget         *table_grid;
	GtkWidget         *label_account;
	GtkWidget         *account_chooser;
	GtkWidget         *label_server;
	GtkWidget         *entry_server;
	GtkWidget         *label_room;
	GtkWidget         *entry_room;
	GtkWidget         *expander_browse;
	GtkWidget         *hbox_expander;
	GtkWidget         *throbber;
	GtkWidget         *treeview;
	GtkTreeModel      *model;
	GtkWidget         *button_join;
	GtkWidget         *label_error_message;
	GtkWidget         *viewport_error;

	GSettings         *gsettings;
} EmpathyNewChatroomDialog;

enum {
	COL_NEED_PASSWORD,
	COL_INVITE_ONLY,
	COL_NAME,
	COL_ROOM,
	COL_MEMBERS,
	COL_MEMBERS_INT,
	COL_TOOLTIP,
	COL_COUNT
};

static void     new_chatroom_dialog_response_cb                     (GtkWidget               *widget,
								     gint                     response,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_destroy_cb                      (GtkWidget               *widget,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_model_setup                     (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_model_add_columns               (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_update_widgets                  (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_account_changed_cb              (GtkComboBox              *combobox,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_account_ready_cb                (EmpathyAccountChooser   *combobox,
                                                                     EmpathyNewChatroomDialog  *dialog);
static void     new_chatroom_dialog_roomlist_destroy_cb             (EmpathyTpRoomlist        *room_list,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_new_room_cb                     (EmpathyTpRoomlist        *room_list,
								     EmpathyChatroom          *chatroom,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_listing_cb                      (EmpathyTpRoomlist        *room_list,
								     gpointer                  unused,
								     EmpathyNewChatroomDialog *dialog);
static void     start_listing_error_cb                              (EmpathyTpRoomlist        *room_list,
								     GError                   *error,
								     EmpathyNewChatroomDialog *dialog);
static void     stop_listing_error_cb                               (EmpathyTpRoomlist        *room_list,
								     GError                   *error,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_model_clear                     (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_model_row_activated_cb          (GtkTreeView             *tree_view,
								     GtkTreePath             *path,
								     GtkTreeViewColumn       *column,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_model_selection_changed         (GtkTreeSelection        *selection,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_join                            (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_entry_changed_cb                (GtkWidget               *entry,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_browse_start                    (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_browse_stop                     (EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_entry_server_activate_cb        (GtkWidget               *widget,
								     EmpathyNewChatroomDialog *dialog);
static void     new_chatroom_dialog_expander_browse_activate_cb     (GtkWidget               *widget,
								     EmpathyNewChatroomDialog *dialog);
static gboolean new_chatroom_dialog_entry_server_focus_out_cb       (GtkWidget               *widget,
								     GdkEventFocus           *event,
								     EmpathyNewChatroomDialog *dialog);
static void	new_chatroom_dialog_button_close_error_clicked_cb   (GtkButton                *button,
								     EmpathyNewChatroomDialog *dialog);

static EmpathyNewChatroomDialog *dialog_p = NULL;


void
empathy_new_chatroom_dialog_show (GtkWindow *parent)
{
	EmpathyNewChatroomDialog *dialog;
	GtkBuilder               *gui;
	GtkSizeGroup             *size_group;
	gchar                    *filename;

	if (dialog_p) {
		gtk_window_present (GTK_WINDOW (dialog_p->window));
		return;
	}

	dialog_p = dialog = g_new0 (EmpathyNewChatroomDialog, 1);

	filename = empathy_file_lookup ("empathy-new-chatroom-dialog.ui", "src");
	gui = empathy_builder_get_file (filename,
				       "new_chatroom_dialog", &dialog->window,
				       "table_grid", &dialog->table_grid,
				       "label_account", &dialog->label_account,
				       "label_server", &dialog->label_server,
				       "label_room", &dialog->label_room,
				       "entry_server", &dialog->entry_server,
				       "entry_room", &dialog->entry_room,
				       "treeview", &dialog->treeview,
				       "button_join", &dialog->button_join,
				       "expander_browse", &dialog->expander_browse,
				       "hbox_expander", &dialog->hbox_expander,
				       "label_error_message", &dialog->label_error_message,
				       "viewport_error", &dialog->viewport_error,
				       NULL);
	g_free (filename);

	empathy_builder_connect (gui, dialog,
			      "new_chatroom_dialog", "response", new_chatroom_dialog_response_cb,
			      "new_chatroom_dialog", "destroy", new_chatroom_dialog_destroy_cb,
			      "entry_server", "changed", new_chatroom_dialog_entry_changed_cb,
			      "entry_server", "activate", new_chatroom_dialog_entry_server_activate_cb,
			      "entry_server", "focus-out-event", new_chatroom_dialog_entry_server_focus_out_cb,
			      "entry_room", "changed", new_chatroom_dialog_entry_changed_cb,
			      "expander_browse", "activate", new_chatroom_dialog_expander_browse_activate_cb,
			      "button_close_error", "clicked", new_chatroom_dialog_button_close_error_clicked_cb,
			      NULL);

	g_object_unref (gui);

	g_object_add_weak_pointer (G_OBJECT (dialog->window), (gpointer) &dialog_p);

	/* Label alignment */
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	gtk_size_group_add_widget (size_group, dialog->label_account);
	gtk_size_group_add_widget (size_group, dialog->label_server);
	gtk_size_group_add_widget (size_group, dialog->label_room);

	g_object_unref (size_group);

	/* Set up chatrooms treeview */
	new_chatroom_dialog_model_setup (dialog);

	/* Add throbber */
	dialog->throbber = gtk_spinner_new ();
	gtk_box_pack_start (GTK_BOX (dialog->hbox_expander), dialog->throbber,
		TRUE, TRUE, 0);

	dialog->gsettings = g_settings_new (EMPATHY_PREFS_CHAT_SCHEMA);

	/* Account chooser for custom */
	dialog->account_chooser = empathy_account_chooser_new ();
	empathy_account_chooser_set_filter (EMPATHY_ACCOUNT_CHOOSER (dialog->account_chooser),
					    empathy_account_chooser_filter_supports_chatrooms,
					    NULL);
	gtk_grid_attach (GTK_GRID (dialog->table_grid),
				   dialog->account_chooser,
				   1, 0, 1, 1);
	gtk_widget_show (dialog->account_chooser);

	g_signal_connect (EMPATHY_ACCOUNT_CHOOSER (dialog->account_chooser), "ready",
			  G_CALLBACK (new_chatroom_dialog_account_ready_cb),
			  dialog);
	g_signal_connect (GTK_COMBO_BOX (dialog->account_chooser), "changed",
			  G_CALLBACK (new_chatroom_dialog_account_changed_cb),
			  dialog);
	new_chatroom_dialog_account_changed_cb (GTK_COMBO_BOX (dialog->account_chooser),
						dialog);

	if (parent) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog->window),
					      GTK_WINDOW (parent));
	}

	gtk_widget_show (dialog->window);
}

static void
new_chatroom_dialog_store_last_account (GSettings             *gsettings,
                                        EmpathyAccountChooser *account_chooser)
{
	TpAccount   *account;
	const char *account_path;

	account = empathy_account_chooser_get_account (account_chooser);
	if (account == NULL)
		return;

	account_path = tp_proxy_get_object_path (account);
	DEBUG ("Storing account path '%s'", account_path);

	g_settings_set (gsettings, EMPATHY_PREFS_CHAT_ROOM_LAST_ACCOUNT,
	                "o", account_path);
}

static void
new_chatroom_dialog_response_cb (GtkWidget               *widget,
				 gint                     response,
				 EmpathyNewChatroomDialog *dialog)
{
	if (response == GTK_RESPONSE_OK) {
		new_chatroom_dialog_join (dialog);
		new_chatroom_dialog_store_last_account (dialog->gsettings,
		                                        EMPATHY_ACCOUNT_CHOOSER (dialog->account_chooser));
	}

	gtk_widget_destroy (widget);
}

static void
new_chatroom_dialog_destroy_cb (GtkWidget               *widget,
				EmpathyNewChatroomDialog *dialog)
{
	if (dialog->room_list) {
		g_object_unref (dialog->room_list);
	}
  	g_object_unref (dialog->model);

	if (dialog->account != NULL) {
		g_signal_handler_disconnect (dialog->account, dialog->status_changed_id);
		g_object_unref (dialog->account);
	}

	g_clear_object (&dialog->gsettings);

	g_free (dialog);
}

static void
new_chatroom_dialog_model_setup (EmpathyNewChatroomDialog *dialog)
{
	GtkTreeView      *view;
	GtkListStore     *store;
	GtkTreeSelection *selection;

	/* View */
	view = GTK_TREE_VIEW (dialog->treeview);

	g_signal_connect (view, "row-activated",
			  G_CALLBACK (new_chatroom_dialog_model_row_activated_cb),
			  dialog);

	/* Store/Model */
	store = gtk_list_store_new (COL_COUNT,
				    G_TYPE_STRING,       /* Invite */
				    G_TYPE_STRING,       /* Password */
				    G_TYPE_STRING,       /* Name */
				    G_TYPE_STRING,       /* Room */
				    G_TYPE_STRING,       /* Member count */
				    G_TYPE_INT,          /* Member count int */
				    G_TYPE_STRING);      /* Tool tip */

	dialog->model = GTK_TREE_MODEL (store);
	gtk_tree_view_set_model (view, dialog->model);
	gtk_tree_view_set_tooltip_column (view, COL_TOOLTIP);
	gtk_tree_view_set_search_column (view, COL_NAME);

	/* Selection */
	selection = gtk_tree_view_get_selection (view);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      COL_NAME, GTK_SORT_ASCENDING);

	g_signal_connect (selection, "changed",
			  G_CALLBACK (new_chatroom_dialog_model_selection_changed),
			  dialog);

	/* Columns */
	new_chatroom_dialog_model_add_columns (dialog);
}

static void
new_chatroom_dialog_model_add_columns (EmpathyNewChatroomDialog *dialog)
{
	GtkTreeView       *view;
	GtkTreeViewColumn *column;
	GtkCellRenderer   *cell;
	gint               width, height;

	gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &width, &height);

	view = GTK_TREE_VIEW (dialog->treeview);

	cell = gtk_cell_renderer_pixbuf_new ();
	g_object_set (cell,
		      "width", width,
		      "height", height,
		      "stock-size", GTK_ICON_SIZE_MENU,
		      NULL);
	column = gtk_tree_view_column_new_with_attributes (NULL,
		                                           cell,
		                                           "stock-id", COL_INVITE_ONLY,
		                                           NULL);

	gtk_tree_view_column_set_sort_column_id (column, COL_INVITE_ONLY);
	gtk_tree_view_append_column (view, column);

	column = gtk_tree_view_column_new_with_attributes (NULL,
		                                           cell,
		                                           "stock-id", COL_NEED_PASSWORD,
		                                           NULL);

	gtk_tree_view_column_set_sort_column_id (column, COL_NEED_PASSWORD);
	gtk_tree_view_append_column (view, column);

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell,
		      "xpad", (guint) 4,
		      "ypad", (guint) 2,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      NULL);

	column = gtk_tree_view_column_new_with_attributes (_("Chat Room"),
		                                           cell,
		                                           "text", COL_NAME,
		                                           NULL);

	gtk_tree_view_column_set_sort_column_id (column, COL_NAME);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (view, column);

	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell,
		      "xpad", (guint) 4,
		      "ypad", (guint) 2,
		      "ellipsize", PANGO_ELLIPSIZE_END,
		      "alignment", PANGO_ALIGN_RIGHT,
		      NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Members"),
		                                           cell,
		                                           "text", COL_MEMBERS,
		                                           NULL);

	gtk_tree_view_column_set_sort_column_id (column, COL_MEMBERS_INT);
	gtk_tree_view_append_column (view, column);
}

static void
update_join_button_sensitivity (EmpathyNewChatroomDialog *dialog)
{
	const gchar           *room;
	const gchar	      *protocol;
	gboolean               sensitive = FALSE;


	room = gtk_entry_get_text (GTK_ENTRY (dialog->entry_room));
	protocol = tp_account_get_protocol (dialog->account);
	if (EMP_STR_EMPTY (room))
		goto out;

	if (!tp_strdiff (protocol, "irc") && (!tp_strdiff (room, "#") ||
	                                      !tp_strdiff (room, "&")))
	{
		goto out;
	}

	if (dialog->account == NULL)
		goto out;

	if (tp_account_get_connection_status (dialog->account, NULL) !=
		      TP_CONNECTION_STATUS_CONNECTED)
		goto out;

	sensitive = TRUE;

out:
	gtk_widget_set_sensitive (dialog->button_join, sensitive);
}

static void
new_chatroom_dialog_update_widgets (EmpathyNewChatroomDialog *dialog)
{
	const gchar           *protocol;

	if (dialog->account == NULL)
		return;

	protocol = tp_account_get_protocol (dialog->account);

	gtk_entry_set_text (GTK_ENTRY (dialog->entry_server), "");

	/* hardcode here known protocols */
	if (strcmp (protocol, "jabber") == 0) {
		gtk_widget_set_sensitive (dialog->entry_server, TRUE);
	}
	else if (strcmp (protocol, "local-xmpp") == 0) {
		gtk_widget_set_sensitive (dialog->entry_server, FALSE);
	}
	else if (strcmp (protocol, "irc") == 0) {
		gtk_widget_set_sensitive (dialog->entry_server, FALSE);
	}
	else {
		gtk_widget_set_sensitive (dialog->entry_server, TRUE);
	}

	if (!tp_strdiff (protocol, "irc"))
		gtk_entry_set_text (GTK_ENTRY (dialog->entry_room), "#");
	else
		gtk_entry_set_text (GTK_ENTRY (dialog->entry_room), "");

	update_join_button_sensitivity (dialog);

	/* Final set up of the dialog */
	gtk_widget_grab_focus (dialog->entry_room);
	gtk_editable_set_position (GTK_EDITABLE (dialog->entry_room), -1);
}

static void
account_status_changed_cb (TpAccount *account,
			    guint old_status,
			    guint new_status,
			    guint reason,
			    gchar *dbus_error_name,
			    GHashTable *details,
			    EmpathyNewChatroomDialog *self)
{
	update_join_button_sensitivity (self);
}

static void
new_chatroom_dialog_select_last_account (GSettings             *gsettings,
                                         EmpathyAccountChooser *account_chooser)
{
	const gchar          *account_path;
	TpAccountManager      *manager;
	TpSimpleClientFactory *factory;
	TpAccount             *account;
	TpConnectionStatus    status;

	account_path = g_settings_get_string (gsettings, EMPATHY_PREFS_CHAT_ROOM_LAST_ACCOUNT);
	DEBUG ("Selecting account path '%s'", account_path);

	manager =  tp_account_manager_dup ();
	factory = tp_proxy_get_factory (manager);
	account = tp_simple_client_factory_ensure_account (factory,
	                                                   account_path,
	                                                   NULL,
	                                                   NULL);

	if (account != NULL) {
		status = tp_account_get_connection_status (account, NULL);
		if (status == TP_CONNECTION_STATUS_CONNECTED) {
			empathy_account_chooser_set_account (account_chooser,
			                                     account);
		}
		g_object_unref (account);
	}
	g_object_unref (manager);
}

static void
new_chatroom_dialog_account_ready_cb (EmpathyAccountChooser    *combobox,
                                      EmpathyNewChatroomDialog *dialog)
{
	new_chatroom_dialog_select_last_account (dialog->gsettings, combobox);
}

static void
new_chatroom_dialog_account_changed_cb (GtkComboBox             *combobox,
					EmpathyNewChatroomDialog *dialog)
{
	EmpathyAccountChooser *account_chooser;
	gboolean               listing = FALSE;
	gboolean               expanded = FALSE;
	TpConnection          *connection;
	TpCapabilities *caps;

	if (dialog->room_list) {
		g_object_unref (dialog->room_list);
		dialog->room_list = NULL;
	}

	gtk_spinner_stop (GTK_SPINNER (dialog->throbber));
	gtk_widget_hide (dialog->throbber);
	new_chatroom_dialog_model_clear (dialog);

	if (dialog->account != NULL) {
		g_signal_handler_disconnect (dialog->account, dialog->status_changed_id);
		g_object_unref (dialog->account);
	}

	account_chooser = EMPATHY_ACCOUNT_CHOOSER (dialog->account_chooser);
	dialog->account = empathy_account_chooser_dup_account (account_chooser);
	connection = empathy_account_chooser_get_connection (account_chooser);
	if (dialog->account == NULL)
		goto out;

	dialog->status_changed_id = g_signal_connect (dialog->account,
		      "status-changed", G_CALLBACK (account_status_changed_cb), dialog);

	/* empathy_account_chooser_filter_supports_chatrooms ensures that the
	* account has a connection and CAPABILITIES has been prepared. */
	g_assert (connection != NULL);
	g_assert (tp_proxy_is_prepared (connection,
		TP_CONNECTION_FEATURE_CAPABILITIES));
	caps = tp_connection_get_capabilities (connection);

	if (tp_capabilities_supports_room_list (caps, NULL)) {
		/* Roomlist channels are supported */
		dialog->room_list = empathy_tp_roomlist_new (dialog->account);
	}
	else {
		dialog->room_list = NULL;
	}

	if (dialog->room_list) {
		g_signal_connect (dialog->room_list, "destroy",
				  G_CALLBACK (new_chatroom_dialog_roomlist_destroy_cb),
				  dialog);
		g_signal_connect (dialog->room_list, "new-room",
				  G_CALLBACK (new_chatroom_dialog_new_room_cb),
				  dialog);
		g_signal_connect (dialog->room_list, "notify::is-listing",
				  G_CALLBACK (new_chatroom_dialog_listing_cb),
				  dialog);
		g_signal_connect (dialog->room_list, "error::start",
				  G_CALLBACK (start_listing_error_cb),
				  dialog);
		g_signal_connect (dialog->room_list, "error::stop",
				  G_CALLBACK (stop_listing_error_cb),
				  dialog);

		expanded = gtk_expander_get_expanded (GTK_EXPANDER (dialog->expander_browse));
		if (expanded) {
			gtk_widget_hide (dialog->viewport_error);
			gtk_widget_set_sensitive (dialog->treeview, TRUE);
			new_chatroom_dialog_browse_start (dialog);
		}

		listing = empathy_tp_roomlist_is_listing (dialog->room_list);
		if (listing) {
			gtk_spinner_start (GTK_SPINNER (dialog->throbber));
			gtk_widget_show (dialog->throbber);
		}
	}

	gtk_widget_set_sensitive (dialog->expander_browse, dialog->room_list != NULL);

out:
	new_chatroom_dialog_update_widgets (dialog);
}

static void
new_chatroom_dialog_button_close_error_clicked_cb   (GtkButton                *button,
						     EmpathyNewChatroomDialog *dialog)
{
	gtk_widget_hide (dialog->viewport_error);
}

static void
new_chatroom_dialog_roomlist_destroy_cb (EmpathyTpRoomlist        *room_list,
					 EmpathyNewChatroomDialog *dialog)
{
	g_object_unref (dialog->room_list);
	dialog->room_list = NULL;
}

static void
new_chatroom_dialog_new_room_cb (EmpathyTpRoomlist        *room_list,
				 EmpathyChatroom          *chatroom,
				 EmpathyNewChatroomDialog *dialog)
{
	GtkListStore     *store;
	GtkTreeIter       iter;
	gchar            *members;
	gchar            *tooltip;
	const gchar      *need_password;
	const gchar      *invite_only;
	gchar            *tmp;

	DEBUG ("New chatroom listed: %s (%s)",
		empathy_chatroom_get_name (chatroom),
		empathy_chatroom_get_room (chatroom));

	/* Add to model */
	store = GTK_LIST_STORE (dialog->model);
	members = g_strdup_printf ("%d", empathy_chatroom_get_members_count (chatroom));
	tmp = g_strdup_printf ("<b>%s</b>", empathy_chatroom_get_name (chatroom));
	/* Translators: Room/Join's roomlist tooltip. Parameters are a channel name,
	yes/no, yes/no and a number. */
	tooltip = g_strdup_printf (_("%s\nInvite required: %s\nPassword required: %s\nMembers: %s"),
		tmp,
		empathy_chatroom_get_invite_only (chatroom) ? _("Yes") : _("No"),
		empathy_chatroom_get_need_password (chatroom) ? _("Yes") : _("No"),
		members);
	g_free (tmp);
	invite_only = (empathy_chatroom_get_invite_only (chatroom) ?
		GTK_STOCK_INDEX : NULL);
	need_password = (empathy_chatroom_get_need_password (chatroom) ?
		GTK_STOCK_DIALOG_AUTHENTICATION : NULL);

	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter,
			    COL_NEED_PASSWORD, need_password,
			    COL_INVITE_ONLY, invite_only,
			    COL_NAME, empathy_chatroom_get_name (chatroom),
			    COL_ROOM, empathy_chatroom_get_room (chatroom),
			    COL_MEMBERS, members,
			    COL_MEMBERS_INT, empathy_chatroom_get_members_count (chatroom),
			    COL_TOOLTIP, tooltip,
			    -1);

	g_free (members);
	g_free (tooltip);
}

static void
start_listing_error_cb (EmpathyTpRoomlist        *room_list,
			GError                   *error,
			EmpathyNewChatroomDialog *dialog)
{
	gtk_label_set_text (GTK_LABEL (dialog->label_error_message), _("Could not start room listing"));
	gtk_widget_show_all (dialog->viewport_error);
	gtk_widget_set_sensitive (dialog->treeview, FALSE);
}

static void
stop_listing_error_cb (EmpathyTpRoomlist        *room_list,
		       GError                   *error,
		       EmpathyNewChatroomDialog *dialog)
{
	gtk_label_set_text (GTK_LABEL (dialog->label_error_message), _("Could not stop room listing"));
	gtk_widget_show_all (dialog->viewport_error);
	gtk_widget_set_sensitive (dialog->treeview, FALSE);
}

static void
new_chatroom_dialog_listing_cb (EmpathyTpRoomlist        *room_list,
				gpointer                  unused,
				EmpathyNewChatroomDialog *dialog)
{
	gboolean listing;

	listing = empathy_tp_roomlist_is_listing (room_list);

	/* Update the throbber */
	if (listing) {
		gtk_spinner_start (GTK_SPINNER (dialog->throbber));
		gtk_widget_show (dialog->throbber);
	} else {
		gtk_spinner_stop (GTK_SPINNER (dialog->throbber));
		gtk_widget_hide (dialog->throbber);
	}
}

static void
new_chatroom_dialog_model_clear (EmpathyNewChatroomDialog *dialog)
{
	GtkListStore *store;

	store = GTK_LIST_STORE (dialog->model);
	gtk_list_store_clear (store);
}

static void
new_chatroom_dialog_model_row_activated_cb (GtkTreeView             *tree_view,
					    GtkTreePath             *path,
					    GtkTreeViewColumn       *column,
					    EmpathyNewChatroomDialog *dialog)
{
	gtk_widget_activate (dialog->button_join);
}

static void
new_chatroom_dialog_model_selection_changed (GtkTreeSelection         *selection,
					     EmpathyNewChatroomDialog *dialog)
{
	GtkTreeModel *model;
	GtkTreeIter   iter;
	gchar        *room = NULL;
	gchar        *server = NULL;

	if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
		return;
	}

	gtk_tree_model_get (model, &iter, COL_ROOM, &room, -1);
	server = strstr (room, "@");
	if (server) {
		*server = '\0';
		server++;
	}

	gtk_entry_set_text (GTK_ENTRY (dialog->entry_server), server ? server : "");
	gtk_entry_set_text (GTK_ENTRY (dialog->entry_room), room ? room : "");

	g_free (room);
}

static void
new_chatroom_dialog_join (EmpathyNewChatroomDialog *dialog)
{
	EmpathyAccountChooser *account_chooser;
	TpAccount *account;
	const gchar           *room;
	const gchar           *server = NULL;
	gchar                 *room_name = NULL;

	room = gtk_entry_get_text (GTK_ENTRY (dialog->entry_room));
	server = gtk_entry_get_text (GTK_ENTRY (dialog->entry_server));

	account_chooser = EMPATHY_ACCOUNT_CHOOSER (dialog->account_chooser);
	account = empathy_account_chooser_get_account (account_chooser);

	if (!EMP_STR_EMPTY (server)) {
		room_name = g_strconcat (room, "@", server, NULL);
	} else {
		room_name = g_strdup (room);
	}

	g_strstrip (room_name);

	DEBUG ("Requesting channel for '%s'", room_name);

	empathy_join_muc (account, room_name, empathy_get_current_action_time ());

	g_free (room_name);
}

static void
new_chatroom_dialog_entry_changed_cb (GtkWidget                *entry,
				      EmpathyNewChatroomDialog *dialog)
{
	if (entry == dialog->entry_room) {
		update_join_button_sensitivity (dialog);

		/* FIXME: Select the room in the list */
	}
}

static void
new_chatroom_dialog_browse_start (EmpathyNewChatroomDialog *dialog)
{
	new_chatroom_dialog_model_clear (dialog);
	if (dialog->room_list) {
		empathy_tp_roomlist_start (dialog->room_list);
	}
}

static void
new_chatroom_dialog_browse_stop (EmpathyNewChatroomDialog *dialog)
{
	if (dialog->room_list) {
		empathy_tp_roomlist_stop (dialog->room_list);
	}
}

static void
new_chatroom_dialog_entry_server_activate_cb (GtkWidget                *widget,
					      EmpathyNewChatroomDialog  *dialog)
{
	new_chatroom_dialog_browse_start (dialog);
}

static void
new_chatroom_dialog_expander_browse_activate_cb (GtkWidget               *widget,
						 EmpathyNewChatroomDialog *dialog)
{
	gboolean expanded;

	expanded = gtk_expander_get_expanded (GTK_EXPANDER (widget));
	if (expanded) {
		new_chatroom_dialog_browse_stop (dialog);
		gtk_window_set_resizable (GTK_WINDOW (dialog->window), FALSE);
	} else {
		gtk_widget_hide (dialog->viewport_error);
		gtk_widget_set_sensitive (dialog->treeview, TRUE);
		new_chatroom_dialog_browse_start (dialog);
		gtk_window_set_resizable (GTK_WINDOW (dialog->window), TRUE);
	}
}

static gboolean
new_chatroom_dialog_entry_server_focus_out_cb (GtkWidget               *widget,
					       GdkEventFocus           *event,
					       EmpathyNewChatroomDialog *dialog)
{
	gboolean expanded;

	expanded = gtk_expander_get_expanded (GTK_EXPANDER (dialog->expander_browse));
	if (expanded) {
		new_chatroom_dialog_browse_start (dialog);
	}
	return FALSE;
}
