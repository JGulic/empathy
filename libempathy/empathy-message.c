/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2007 Imendio AB
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
 * Authors: Mikael Hallendal <micke@imendio.com>
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>

#include <telepathy-glib/util.h>
#include <telepathy-glib/account.h>
#include <telepathy-glib/account-manager.h>

#include <telepathy-logger/entity.h>
#include <telepathy-logger/event.h>
#include <telepathy-logger/text-event.h>
#ifdef HAVE_CALL_LOGS
# include <telepathy-logger/call-event.h>
#endif

#include "empathy-client-factory.h"
#include "empathy-message.h"
#include "empathy-utils.h"
#include "empathy-enum-types.h"

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyMessage)
typedef struct {
	TpMessage *tp_message;
	TpChannelTextMessageType  type;
	EmpathyContact           *sender;
	EmpathyContact           *receiver;
	gchar                    *token;
	gchar                    *supersedes;
	gchar                    *body;
	gint64                    timestamp;
	gint64                    original_timestamp;
	gboolean                  is_backlog;
	guint                     id;
	gboolean                  incoming;
	TpChannelTextMessageFlags flags;
} EmpathyMessagePriv;

static void empathy_message_finalize   (GObject            *object);
static void message_get_property      (GObject            *object,
				       guint               param_id,
				       GValue             *value,
				       GParamSpec         *pspec);
static void message_set_property      (GObject            *object,
				       guint               param_id,
				       const GValue       *value,
				       GParamSpec         *pspec);

G_DEFINE_TYPE (EmpathyMessage, empathy_message, G_TYPE_OBJECT);

enum {
	PROP_0,
	PROP_TYPE,
	PROP_SENDER,
	PROP_RECEIVER,
	PROP_TOKEN,
	PROP_SUPERSEDES,
	PROP_BODY,
	PROP_TIMESTAMP,
	PROP_ORIGINAL_TIMESTAMP,
	PROP_IS_BACKLOG,
	PROP_INCOMING,
	PROP_FLAGS,
	PROP_TP_MESSAGE,
};

static void
empathy_message_class_init (EmpathyMessageClass *class)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (class);
	object_class->finalize     = empathy_message_finalize;
	object_class->get_property = message_get_property;
	object_class->set_property = message_set_property;

	g_object_class_install_property (object_class,
					 PROP_TYPE,
					 g_param_spec_uint ("type",
							    "Message Type",
							    "The type of message",
							    TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,
							    TP_CHANNEL_TEXT_MESSAGE_TYPE_AUTO_REPLY,
							    TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL,
							    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							    G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_SENDER,
					 g_param_spec_object ("sender",
							      "Message Sender",
							      "The sender of the message",
							      EMPATHY_TYPE_CONTACT,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
					 PROP_RECEIVER,
					 g_param_spec_object ("receiver",
							      "Message Receiver",
							      "The receiver of the message",
							      EMPATHY_TYPE_CONTACT,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (object_class,
					 PROP_TOKEN,
					 g_param_spec_string ("token",
						 	      "Message Token",
							      "The message-token",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_SUPERSEDES,
					 g_param_spec_string ("supersedes",
							      "Supersedes Token",
							      "The message-token this message supersedes",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_BODY,
					 g_param_spec_string ("body",
							      "Message Body",
							      "The content of the message",
							      NULL,
							      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							      G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_TIMESTAMP,
					 g_param_spec_int64 ("timestamp",
							    "timestamp",
							    "timestamp",
							    G_MININT64, G_MAXINT64, 0,
							    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							    G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_ORIGINAL_TIMESTAMP,
					 g_param_spec_int64 ("original-timestamp",
							     "Original Timestamp",
							     "Timestamp of the original message",
							     G_MININT64, G_MAXINT64, 0,
							     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
					 PROP_IS_BACKLOG,
					 g_param_spec_boolean ("is-backlog",
							       "History message",
							       "If the message belongs to history",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_CONSTRUCT_ONLY));


	g_object_class_install_property (object_class,
					 PROP_INCOMING,
					 g_param_spec_boolean ("incoming",
							       "Incoming",
							       "If this is an incoming (as opposed to sent) message",
							       FALSE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_FLAGS,
					 g_param_spec_uint ("flags",
							       "Flags",
							       "The TpChannelTextMessageFlags of this message",
							       0, G_MAXUINT, 0,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_TP_MESSAGE,
					 g_param_spec_object ("tp-message",
							       "TpMessage",
							       "The TpMessage of this message",
							       TP_TYPE_MESSAGE,
							       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |
							       G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (EmpathyMessagePriv));

}

static void
empathy_message_init (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv = G_TYPE_INSTANCE_GET_PRIVATE (message,
		EMPATHY_TYPE_MESSAGE, EmpathyMessagePriv);

	message->priv = priv;
	priv->timestamp = empathy_time_get_current ();
}

static void
empathy_message_finalize (GObject *object)
{
	EmpathyMessagePriv *priv;

	priv = GET_PRIV (object);

	if (priv->sender) {
		g_object_unref (priv->sender);
	}
	if (priv->receiver) {
		g_object_unref (priv->receiver);
	}

	if (priv->tp_message) {
		g_object_unref (priv->tp_message);
	}

	g_free (priv->token);
	g_free (priv->supersedes);
	g_free (priv->body);

	G_OBJECT_CLASS (empathy_message_parent_class)->finalize (object);
}

static void
message_get_property (GObject    *object,
		      guint       param_id,
		      GValue     *value,
		      GParamSpec *pspec)
{
	EmpathyMessagePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TYPE:
		g_value_set_uint (value, priv->type);
		break;
	case PROP_SENDER:
		g_value_set_object (value, priv->sender);
		break;
	case PROP_RECEIVER:
		g_value_set_object (value, priv->receiver);
		break;
	case PROP_TOKEN:
		g_value_set_string (value, priv->token);
		break;
	case PROP_SUPERSEDES:
		g_value_set_string (value, priv->supersedes);
		break;
	case PROP_BODY:
		g_value_set_string (value, priv->body);
		break;
	case PROP_TIMESTAMP:
		g_value_set_int64 (value, priv->timestamp);
		break;
	case PROP_ORIGINAL_TIMESTAMP:
		g_value_set_int64 (value, priv->original_timestamp);
		break;
	case PROP_IS_BACKLOG:
		g_value_set_boolean (value, priv->is_backlog);
		break;
	case PROP_INCOMING:
		g_value_set_boolean (value, priv->incoming);
		break;
	case PROP_FLAGS:
		g_value_set_uint (value, priv->flags);
		break;
	case PROP_TP_MESSAGE:
		g_value_set_object (value, priv->tp_message);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

static void
message_set_property (GObject      *object,
		      guint         param_id,
		      const GValue *value,
		      GParamSpec   *pspec)
{
	EmpathyMessagePriv *priv;

	priv = GET_PRIV (object);

	switch (param_id) {
	case PROP_TYPE:
		priv->type = g_value_get_uint (value);
		break;
	case PROP_SENDER:
		empathy_message_set_sender (EMPATHY_MESSAGE (object),
					   EMPATHY_CONTACT (g_value_get_object (value)));
		break;
	case PROP_RECEIVER:
		empathy_message_set_receiver (EMPATHY_MESSAGE (object),
					     EMPATHY_CONTACT (g_value_get_object (value)));
		break;
	case PROP_TOKEN:
		g_assert (priv->token == NULL); /* construct only */
		priv->token = g_value_dup_string (value);
		break;
	case PROP_SUPERSEDES:
		g_assert (priv->supersedes == NULL); /* construct only */
		priv->supersedes = g_value_dup_string (value);
		break;
	case PROP_BODY:
		g_assert (priv->body == NULL); /* construct only */
		priv->body = g_value_dup_string (value);
		break;
	case PROP_TIMESTAMP:
		priv->timestamp = g_value_get_int64 (value);
		if (priv->timestamp <= 0)
			priv->timestamp = empathy_time_get_current ();
		break;
	case PROP_ORIGINAL_TIMESTAMP:
		priv->original_timestamp = g_value_get_int64 (value);
		break;
	case PROP_IS_BACKLOG:
		priv->is_backlog = g_value_get_boolean (value);
		break;
	case PROP_INCOMING:
		priv->incoming = g_value_get_boolean (value);
		break;
	case PROP_FLAGS:
		priv->flags = g_value_get_uint (value);
		break;
	case PROP_TP_MESSAGE:
		priv->tp_message = g_value_dup_object (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	};
}

EmpathyMessage *
empathy_message_from_tpl_log_event (TplEvent *logevent)
{
	EmpathyMessage *retval = NULL;
	EmpathyClientFactory *factory;
	TpAccount *account = NULL;
	TplEntity *receiver = NULL;
	TplEntity *sender = NULL;
	gchar *body = NULL;
	const gchar *token = NULL, *supersedes = NULL;
	EmpathyContact *contact;
	TpChannelTextMessageType type = TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL;
	gint64 timestamp, original_timestamp = 0;

	g_return_val_if_fail (TPL_IS_EVENT (logevent), NULL);

	factory = empathy_client_factory_dup ();
	/* FIXME Currently Empathy shows in the log viewer only valid accounts, so it
	 * won't be selected any non-existing (ie removed) account.
	 * When #610455 will be fixed, calling tp_account_manager_ensure_account ()
	 * might add a not existing account to the AM. tp_account_new () probably
	 * will be the best way to handle it.
	 * Note: When creating an EmpathyContact from a TplEntity instance, the
	 * TpAccount is passed *only* to let EmpathyContact be able to retrieve the
	 * avatar (contact_get_avatar_filename () need a TpAccount).
	 * If the way EmpathyContact stores the avatar is changes, it might not be
	 * needed anymore any TpAccount passing and the following call will be
	 * useless */
	account = tp_simple_client_factory_ensure_account (
			TP_SIMPLE_CLIENT_FACTORY (factory),
			tpl_event_get_account_path (logevent), NULL, NULL);
	g_object_unref (factory);

	if (TPL_IS_TEXT_EVENT (logevent)) {
		TplTextEvent *textevent = TPL_TEXT_EVENT (logevent);

		supersedes = tpl_text_event_get_supersedes_token (textevent);

		/* tp-logger is kind of messy in that instead of having
		 * timestamp and original-timestamp like Telepathy it has
		 * timestamp (which is the original) and edited-timestamp,
		 * (which is when the message was edited) */
		if (tp_str_empty (supersedes)) {
			/* not an edited message */
			timestamp = tpl_event_get_timestamp (logevent);
		} else {
			/* this is an edited event */
			original_timestamp = tpl_event_get_timestamp (logevent);
			timestamp = tpl_text_event_get_edit_timestamp (textevent);
		}

		body = g_strdup (tpl_text_event_get_message (textevent));

		type = tpl_text_event_get_message_type (TPL_TEXT_EVENT (logevent));
		token = tpl_text_event_get_message_token (textevent);
	}
#ifdef HAVE_CALL_LOGS
	else if (TPL_IS_CALL_EVENT (logevent)) {
		TplCallEvent *call = TPL_CALL_EVENT (logevent);

		timestamp = tpl_event_get_timestamp (logevent);

		if (tpl_call_event_get_end_reason (call) == TPL_CALL_END_REASON_NO_ANSWER)
			body = g_strdup_printf (_("Missed call from %s"),
				tpl_entity_get_alias (tpl_event_get_sender (logevent)));
		else if (tpl_entity_get_entity_type (tpl_event_get_sender (logevent)) == TPL_ENTITY_SELF)
			/* Translators: this is an outgoing call, e.g. 'Called Alice' */
			body = g_strdup_printf (_("Called %s"),
				tpl_entity_get_alias (tpl_event_get_receiver (logevent)));
		else
			body = g_strdup_printf (_("Call from %s"),
				tpl_entity_get_alias (tpl_event_get_sender (logevent)));
	}
#endif
	else {
		/* Unknown event type */
		return NULL;
	}

	receiver = tpl_event_get_receiver (logevent);
	sender = tpl_event_get_sender (logevent);

	retval = g_object_new (EMPATHY_TYPE_MESSAGE,
		"type", type,
		"token", token,
		"supersedes", supersedes,
		"body", body,
		"is-backlog", TRUE,
		"timestamp", timestamp,
		"original-timestamp", original_timestamp,
		NULL);

	if (receiver != NULL) {
		contact = empathy_contact_from_tpl_contact (account, receiver);
		empathy_message_set_receiver (retval, contact);
		g_object_unref (contact);
	}

	if (sender != NULL) {
		contact = empathy_contact_from_tpl_contact (account, sender);
		empathy_message_set_sender (retval, contact);
		g_object_unref (contact);
	}

	g_free (body);

	return retval;
}

TpMessage *
empathy_message_get_tp_message (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->tp_message;
}

TpChannelTextMessageType
empathy_message_get_tptype (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message),
			      TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL);

	priv = GET_PRIV (message);

	return priv->type;
}

EmpathyContact *
empathy_message_get_sender (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->sender;
}

void
empathy_message_set_sender (EmpathyMessage *message, EmpathyContact *contact)
{
	EmpathyMessagePriv *priv;
	EmpathyContact     *old_sender;

	g_return_if_fail (EMPATHY_IS_MESSAGE (message));
	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	priv = GET_PRIV (message);

	old_sender = priv->sender;
	priv->sender = g_object_ref (contact);

	if (old_sender) {
		g_object_unref (old_sender);
	}

	g_object_notify (G_OBJECT (message), "sender");
}

EmpathyContact *
empathy_message_get_receiver (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->receiver;
}

void
empathy_message_set_receiver (EmpathyMessage *message, EmpathyContact *contact)
{
	EmpathyMessagePriv *priv;
	EmpathyContact     *old_receiver;

	g_return_if_fail (EMPATHY_IS_MESSAGE (message));
	g_return_if_fail (EMPATHY_IS_CONTACT (contact));

	priv = GET_PRIV (message);

	old_receiver = priv->receiver;
	priv->receiver = g_object_ref (contact);

	if (old_receiver) {
		g_object_unref (old_receiver);
	}

	g_object_notify (G_OBJECT (message), "receiver");
}

const gchar *
empathy_message_get_token (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->token;
}

const gchar *
empathy_message_get_supersedes (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->supersedes;
}

gboolean
empathy_message_is_edit (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), FALSE);

	priv = GET_PRIV (message);

	return !tp_str_empty (priv->supersedes);
}

const gchar *
empathy_message_get_body (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), NULL);

	priv = GET_PRIV (message);

	return priv->body;
}

gint64
empathy_message_get_timestamp (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), -1);

	priv = GET_PRIV (message);

	return priv->timestamp;
}

gint64
empathy_message_get_original_timestamp (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), -1);

	priv = GET_PRIV (message);

	return priv->original_timestamp;
}

gboolean
empathy_message_is_backlog (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), FALSE);

	priv = GET_PRIV (message);

	return priv->is_backlog;
}

#define IS_SEPARATOR(ch) (ch == ' ' || ch == ',' || ch == '.' || ch == ':')
gboolean
empathy_message_should_highlight (EmpathyMessage *message)
{
	EmpathyContact *contact;
	const gchar   *msg, *to;
	gchar         *cf_msg, *cf_to;
	gchar         *ch;
	gboolean       ret_val;
	TpChannelTextMessageFlags flags;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), FALSE);

	ret_val = FALSE;

	msg = empathy_message_get_body (message);
	if (!msg) {
		return FALSE;
	}

	contact = empathy_message_get_receiver (message);
	if (!contact || !empathy_contact_is_user (contact)) {
		return FALSE;
	}

	to = empathy_contact_get_alias (contact);
	if (!to) {
		return FALSE;
	}

	flags = empathy_message_get_flags (message);
	if (flags & TP_CHANNEL_TEXT_MESSAGE_FLAG_SCROLLBACK) {
		/* FIXME: Ideally we shouldn't highlight scrollback messages only if they
		 * have already been received by the user before (and so are in the logs) */
		return FALSE;
	}

	cf_msg = g_utf8_casefold (msg, -1);
	cf_to = g_utf8_casefold (to, -1);

	ch = strstr (cf_msg, cf_to);
	if (ch == NULL) {
		goto finished;
	}
	if (ch != cf_msg) {
		/* Not first in the message */
		if (!IS_SEPARATOR (*(ch - 1))) {
			goto finished;
		}
	}

	ch = ch + strlen (cf_to);
	if (ch >= cf_msg + strlen (cf_msg)) {
		ret_val = TRUE;
		goto finished;
	}

	if (IS_SEPARATOR (*ch)) {
		ret_val = TRUE;
		goto finished;
	}

finished:
	g_free (cf_msg);
	g_free (cf_to);

	return ret_val;
}

TpChannelTextMessageType
empathy_message_type_from_str (const gchar *type_str)
{
	if (strcmp (type_str, "normal") == 0) {
		return TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL;
	}
	if (strcmp (type_str, "action") == 0) {
		return TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION;
	}
	else if (strcmp (type_str, "notice") == 0) {
		return TP_CHANNEL_TEXT_MESSAGE_TYPE_NOTICE;
	}
	else if (strcmp (type_str, "auto-reply") == 0) {
		return TP_CHANNEL_TEXT_MESSAGE_TYPE_AUTO_REPLY;
	}

	return TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL;
}

const gchar *
empathy_message_type_to_str (TpChannelTextMessageType type)
{
	switch (type) {
	case TP_CHANNEL_TEXT_MESSAGE_TYPE_ACTION:
		return "action";
	case TP_CHANNEL_TEXT_MESSAGE_TYPE_NOTICE:
		return "notice";
	case TP_CHANNEL_TEXT_MESSAGE_TYPE_AUTO_REPLY:
		return "auto-reply";
	case TP_CHANNEL_TEXT_MESSAGE_TYPE_DELIVERY_REPORT:
		return "delivery-report";
	case TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL:
	default:
		return "normal";
	}
}

gboolean
empathy_message_is_incoming (EmpathyMessage *message)
{
	EmpathyMessagePriv *priv = GET_PRIV (message);

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message), FALSE);

	return priv->incoming;
}

gboolean
empathy_message_equal (EmpathyMessage *message1, EmpathyMessage *message2)
{
	EmpathyMessagePriv *priv1;
	EmpathyMessagePriv *priv2;

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message1), FALSE);
	g_return_val_if_fail (EMPATHY_IS_MESSAGE (message2), FALSE);

	priv1 = GET_PRIV (message1);
	priv2 = GET_PRIV (message2);

	if (priv1->timestamp == priv2->timestamp &&
			!tp_strdiff (priv1->body, priv2->body)) {
		return TRUE;
	}

	return FALSE;
}

TpChannelTextMessageFlags
empathy_message_get_flags (EmpathyMessage *self)
{
	EmpathyMessagePriv *priv = GET_PRIV (self);

	g_return_val_if_fail (EMPATHY_IS_MESSAGE (self), 0);

	return priv->flags;
}

EmpathyMessage *
empathy_message_new_from_tp_message (TpMessage *tp_msg,
				     gboolean incoming)
{
	EmpathyMessage *message;
	gchar *body;
	TpChannelTextMessageFlags flags;
	gint64 timestamp;
	gint64 original_timestamp;
	const GHashTable *part = tp_message_peek (tp_msg, 0);
	gboolean is_backlog;

	g_return_val_if_fail (TP_IS_MESSAGE (tp_msg), NULL);

	body = tp_message_to_text (tp_msg, &flags);

	timestamp = tp_message_get_sent_timestamp (tp_msg);
	if (timestamp == 0)
		timestamp = tp_message_get_received_timestamp (tp_msg);

	original_timestamp = tp_asv_get_int64 (part,
		"original-message-received", NULL);

	is_backlog = (flags & TP_CHANNEL_TEXT_MESSAGE_FLAG_SCROLLBACK) ==
		TP_CHANNEL_TEXT_MESSAGE_FLAG_SCROLLBACK;

	message = g_object_new (EMPATHY_TYPE_MESSAGE,
		"body", body,
		"token", tp_message_get_token (tp_msg),
		"supersedes", tp_message_get_supersedes (tp_msg),
		"type", tp_message_get_message_type (tp_msg),
		"timestamp", timestamp,
		"original-timestamp", original_timestamp,
		"flags", flags,
		"is-backlog", is_backlog,
		"incoming", incoming,
		"tp-message", tp_msg,
		NULL);

	g_free (body);
	return message;
}
