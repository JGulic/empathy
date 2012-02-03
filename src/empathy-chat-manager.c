/*
 * empathy-chat-manager.c - Source for EmpathyChatManager
 * Copyright (C) 2010 Collabora Ltd.
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

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/proxy-subclass.h>

#include <libempathy/empathy-chatroom-manager.h>
#include <libempathy/empathy-request-util.h>
#include <libempathy/empathy-utils.h>

#include <libempathy-gtk/empathy-ui-utils.h>

#include "empathy-chat-window.h"

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>

#include "empathy-chat-manager.h"

#include <extensions/extensions.h>

enum {
  CLOSED_CHATS_CHANGED,
  DISPLAYED_CHATS_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];

static void svc_iface_init (gpointer, gpointer);

G_DEFINE_TYPE_WITH_CODE (EmpathyChatManager, empathy_chat_manager,
    G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (EMP_TYPE_SVC_CHAT_MANAGER,
        svc_iface_init)
    )

/* private structure */
typedef struct _EmpathyChatManagerPriv EmpathyChatManagerPriv;

struct _EmpathyChatManagerPriv
{
  EmpathyChatroomManager *chatroom_mgr;
  /* Queue of (ChatData *) representing the closed chats */
  GQueue *closed_queue;

  guint num_displayed_chat;

  /* account path -> (GHashTable<(owned gchar *) contact ID
   *                  -> (owned gchar *) non-NULL message>)
   */
  GHashTable *messages;

  TpBaseClient *handler;
};

#define GET_PRIV(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), EMPATHY_TYPE_CHAT_MANAGER, \
    EmpathyChatManagerPriv))

static EmpathyChatManager *chat_manager_singleton = NULL;

typedef struct
{
  TpAccount *account;
  gchar *id;
  gboolean room;
  gboolean sms;
} ChatData;

static ChatData *
chat_data_new (EmpathyChat *chat)
{
  ChatData *data = NULL;

  data = g_slice_new0 (ChatData);

  data->account = g_object_ref (empathy_chat_get_account (chat));
  data->id = g_strdup (empathy_chat_get_id (chat));
  data->room = empathy_chat_is_room (chat);
  data->sms = empathy_chat_is_sms_channel (chat);

  return data;
}

static void
chat_data_free (ChatData *data)
{
  if (data->account != NULL)
    {
      g_object_unref (data->account);
      data->account = NULL;
    }

  if (data->id != NULL)
    {
      g_free (data->id);
      data->id = NULL;
    }

  g_slice_free (ChatData, data);
}

static void
chat_destroyed_cb (gpointer data,
    GObject *object)
{
  EmpathyChatManager *self = data;
  EmpathyChatManagerPriv *priv = GET_PRIV (self);

  priv->num_displayed_chat--;

  DEBUG ("Chat destroyed; we are now displaying %u chats",
      priv->num_displayed_chat);

  g_signal_emit (self, signals[DISPLAYED_CHATS_CHANGED], 0,
      priv->num_displayed_chat);
}

static void
process_tp_chat (EmpathyChatManager *self,
    EmpathyTpChat *tp_chat,
    TpAccount *account,
    gint64 user_action_time)
{
  EmpathyChatManagerPriv *priv = GET_PRIV (self);
  EmpathyChat *chat = NULL;
  const gchar *id;

  id = empathy_tp_chat_get_id (tp_chat);
  if (!tp_str_empty (id))
    {
      chat = empathy_chat_window_find_chat (account, id,
          tp_text_channel_is_sms_channel ((TpTextChannel *) tp_chat));
    }

  if (chat != NULL)
    {
      empathy_chat_set_tp_chat (chat, tp_chat);
    }
  else
    {
      GHashTable *chats = NULL;

      chat = empathy_chat_new (tp_chat);
      /* empathy_chat_new returns a floating reference as EmpathyChat is
       * a GtkWidget. This reference will be taken by a container
       * (a GtkNotebook) when we'll call empathy_chat_window_present_chat */

      priv->num_displayed_chat++;

      DEBUG ("Chat displayed; we are now displaying %u chat",
          priv->num_displayed_chat);

      g_signal_emit (self, signals[DISPLAYED_CHATS_CHANGED], 0,
          priv->num_displayed_chat);

      /* Set the saved message in the channel if we have one. */
      chats = g_hash_table_lookup (priv->messages,
          tp_proxy_get_object_path (account));

      if (chats != NULL)
        {
          const gchar *msg = g_hash_table_lookup (chats, id);

          if (msg != NULL)
            empathy_chat_set_text (chat, msg);
        }

      g_object_weak_ref ((GObject *) chat, chat_destroyed_cb, self);
    }
  empathy_chat_window_present_chat (chat, user_action_time);

  if (empathy_tp_chat_is_invited (tp_chat, NULL))
    {
      /* We have been invited to the room. Add ourself as member as this
       * channel has been approved. */
      empathy_tp_chat_join (tp_chat);
    }
}

static void
handle_channels (TpSimpleHandler *handler,
    TpAccount *account,
    TpConnection *connection,
    GList *channels,
    GList *requests_satisfied,
    gint64 user_action_time,
    TpHandleChannelsContext *context,
    gpointer user_data)
{
  EmpathyChatManager *self = (EmpathyChatManager *) user_data;
  GList *l;

  for (l = channels; l != NULL; l = g_list_next (l))
    {
      EmpathyTpChat *tp_chat = l->data;

      if (tp_proxy_get_invalidated (tp_chat) != NULL)
        continue;

      if (!EMPATHY_IS_TP_CHAT (tp_chat))
        {
          DEBUG ("Channel %s doesn't implement Messages; can't handle it",
              tp_proxy_get_object_path (tp_chat));
          continue;
        }

      DEBUG ("Now handling channel %s", tp_proxy_get_object_path (tp_chat));

      process_tp_chat (self, tp_chat, account, user_action_time);
    }

  tp_handle_channels_context_accept (context);
}

static void
empathy_chat_manager_init (EmpathyChatManager *self)
{
  EmpathyChatManagerPriv *priv = GET_PRIV (self);
  TpAccountManager *am;
  GError *error = NULL;

  priv->closed_queue = g_queue_new ();
  priv->messages = g_hash_table_new_full (g_str_hash, g_str_equal,
      g_free, (GDestroyNotify) g_hash_table_unref);

  am = tp_account_manager_dup ();

  priv->chatroom_mgr = empathy_chatroom_manager_dup_singleton (NULL);

  /* Text channels handler */
  priv->handler = tp_simple_handler_new_with_am (am, FALSE, FALSE,
      EMPATHY_CHAT_BUS_NAME_SUFFIX, FALSE, handle_channels, self, NULL);

  g_object_unref (am);

  tp_base_client_take_handler_filter (priv->handler, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
        NULL));

  tp_base_client_take_handler_filter (priv->handler, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_ROOM,
        NULL));

  tp_base_client_take_handler_filter (priv->handler, tp_asv_new (
        TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
        TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT, TP_HANDLE_TYPE_NONE,
        NULL));

  if (!tp_base_client_register (priv->handler, &error))
    {
      g_critical ("Failed to register text handler: %s", error->message);
      g_error_free (error);
    }
}

static void
empathy_chat_manager_finalize (GObject *object)
{
  EmpathyChatManager *self = EMPATHY_CHAT_MANAGER (object);
  EmpathyChatManagerPriv *priv = GET_PRIV (self);

  if (priv->closed_queue != NULL)
    {
      g_queue_foreach (priv->closed_queue, (GFunc) chat_data_free, NULL);
      g_queue_free (priv->closed_queue);
      priv->closed_queue = NULL;
    }

  tp_clear_pointer (&priv->messages, g_hash_table_unref);

  tp_clear_object (&priv->handler);
  tp_clear_object (&priv->chatroom_mgr);

  G_OBJECT_CLASS (empathy_chat_manager_parent_class)->finalize (object);
}

static GObject *
empathy_chat_manager_constructor (GType type,
    guint n_construct_params,
    GObjectConstructParam *construct_params)
{
  GObject *retval;

  if (!chat_manager_singleton)
    {
      retval = G_OBJECT_CLASS (empathy_chat_manager_parent_class)->constructor
        (type, n_construct_params, construct_params);

      chat_manager_singleton = EMPATHY_CHAT_MANAGER (retval);
      g_object_add_weak_pointer (retval, (gpointer) &chat_manager_singleton);
    }
  else
    {
      retval = g_object_ref (chat_manager_singleton);
    }

  return retval;
}

static void
empathy_chat_manager_constructed (GObject *obj)
{
  TpDBusDaemon *dbus_daemon;

  dbus_daemon = tp_dbus_daemon_dup (NULL);

  if (dbus_daemon != NULL)
    {
      tp_dbus_daemon_register_object (dbus_daemon,
          "/org/gnome/Empathy/ChatManager", obj);

      g_object_unref (dbus_daemon);
    }
}

static void
empathy_chat_manager_class_init (
  EmpathyChatManagerClass *empathy_chat_manager_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (empathy_chat_manager_class);

  object_class->finalize = empathy_chat_manager_finalize;
  object_class->constructor = empathy_chat_manager_constructor;
  object_class->constructed = empathy_chat_manager_constructed;

  signals[CLOSED_CHATS_CHANGED] =
    g_signal_new ("closed-chats-changed",
        G_TYPE_FROM_CLASS (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        g_cclosure_marshal_generic,
        G_TYPE_NONE,
        1, G_TYPE_UINT, NULL);

  signals[DISPLAYED_CHATS_CHANGED] =
    g_signal_new ("displayed-chats-changed",
        G_TYPE_FROM_CLASS (object_class),
        G_SIGNAL_RUN_LAST,
        0,
        NULL, NULL,
        g_cclosure_marshal_generic,
        G_TYPE_NONE,
        1, G_TYPE_UINT, NULL);

  g_type_class_add_private (empathy_chat_manager_class,
    sizeof (EmpathyChatManagerPriv));
}

EmpathyChatManager *
empathy_chat_manager_dup_singleton (void)
{
  return g_object_new (EMPATHY_TYPE_CHAT_MANAGER, NULL);
}

void
empathy_chat_manager_closed_chat (EmpathyChatManager *self,
    EmpathyChat *chat)
{
  EmpathyChatManagerPriv *priv = GET_PRIV (self);
  ChatData *data;
  GHashTable *chats;
  gchar *message;

  data = chat_data_new (chat);

  DEBUG ("Adding %s to closed queue: %s",
      data->room ? "room" : "contact", data->id);

  g_queue_push_tail (priv->closed_queue, data);

  g_signal_emit (self, signals[CLOSED_CHATS_CHANGED], 0,
      g_queue_get_length (priv->closed_queue));

  /* If there was a message saved from last time it was closed
   * (perhaps by accident?) save it to our hash table so it can be
   * used again when the same chat pops up. Hot. */
  message = empathy_chat_dup_text (chat);

  chats = g_hash_table_lookup (priv->messages,
      tp_proxy_get_object_path (data->account));

  /* Don't create a new hash table if we don't already have one and we
   * don't actually have a message to save. */
  if (chats == NULL && tp_str_empty (message))
    {
      g_free (message);
      return;
    }
  else if (chats == NULL && !tp_str_empty (message))
    {
      chats = g_hash_table_new_full (g_str_hash, g_str_equal,
          g_free, g_free);

      g_hash_table_insert (priv->messages,
          g_strdup (tp_proxy_get_object_path (data->account)),
          chats);
    }

  if (tp_str_empty (message))
    {
      g_hash_table_remove (chats, data->id);
      /* might be '\0' */
      g_free (message);
    }
  else
    {
      /* takes ownership of message */
      g_hash_table_insert (chats, g_strdup (data->id), message);
    }
}

void
empathy_chat_manager_undo_closed_chat (EmpathyChatManager *self,
    gint64 timestamp)
{
  EmpathyChatManagerPriv *priv = GET_PRIV (self);
  ChatData *data;

  data = g_queue_pop_tail (priv->closed_queue);

  if (data == NULL)
    return;

  DEBUG ("Removing %s from closed queue and starting a chat with: %s",
      data->room ? "room" : "contact", data->id);

  if (data->room)
    empathy_join_muc (data->account, data->id, timestamp);
  else if (data->sms)
    empathy_sms_contact_id (data->account, data->id, timestamp,
        NULL, NULL);
  else
    empathy_chat_with_contact_id (data->account, data->id, timestamp,
        NULL, NULL);

  g_signal_emit (self, signals[CLOSED_CHATS_CHANGED], 0,
      g_queue_get_length (priv->closed_queue));

  chat_data_free (data);
}

guint
empathy_chat_manager_get_num_closed_chats (EmpathyChatManager *self)
{
  EmpathyChatManagerPriv *priv = GET_PRIV (self);

  return g_queue_get_length (priv->closed_queue);
}

static void
empathy_chat_manager_dbus_undo_closed_chat (EmpSvcChatManager *manager,
    gint64 timestamp,
    DBusGMethodInvocation *context)
{
  empathy_chat_manager_undo_closed_chat ((EmpathyChatManager *) manager,
      timestamp);

  emp_svc_chat_manager_return_from_undo_closed_chat (context);
}

static void
svc_iface_init (gpointer g_iface,
    gpointer iface_data)
{
  EmpSvcChatManagerClass *klass = (EmpSvcChatManagerClass *) g_iface;

#define IMPLEMENT(x) emp_svc_chat_manager_implement_##x (\
    klass, empathy_chat_manager_dbus_##x)
  IMPLEMENT(undo_closed_chat);
#undef IMPLEMENT
}

void
empathy_chat_manager_call_undo_closed_chat (void)
{
  TpDBusDaemon *dbus_daemon = tp_dbus_daemon_dup (NULL);
  TpProxy *proxy;

  if (dbus_daemon == NULL)
    return;

  proxy = g_object_new (TP_TYPE_PROXY,
      "dbus-daemon", dbus_daemon,
      "dbus-connection", tp_proxy_get_dbus_connection (TP_PROXY (dbus_daemon)),
      "bus-name", EMPATHY_CHAT_BUS_NAME,
      "object-path", "/org/gnome/Empathy/ChatManager",
      NULL);

  tp_proxy_add_interface_by_id (proxy, EMP_IFACE_QUARK_CHAT_MANAGER);

  emp_cli_chat_manager_call_undo_closed_chat (proxy, -1, empathy_get_current_action_time (),
      NULL, NULL, NULL, NULL);

  g_object_unref (proxy);
  g_object_unref (dbus_daemon);
}
