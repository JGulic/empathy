/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2002-2007 Imendio AB
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
 *          Richard Hult <richard@imendio.com>
 *          Martyn Russell <martyn@imendio.com>
 *          Geert-Jan Van den Bogaerde <geertjan@gnome.org>
 *          Xavier Claessens <xclaesse@gmail.com>
 */

#ifndef __EMPATHY_CHAT_H__
#define __EMPATHY_CHAT_H__

#include <gtk/gtk.h>

#include <libempathy/empathy-contact.h>
#include <libempathy/empathy-message.h>
#include <libempathy/empathy-tp-chat.h>

#include "empathy-chat-view.h"

G_BEGIN_DECLS

#define EMPATHY_TYPE_CHAT         (empathy_chat_get_type ())
#define EMPATHY_CHAT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_CHAT, EmpathyChat))
#define EMPATHY_CHAT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), EMPATHY_TYPE_CHAT, EmpathyChatClass))
#define EMPATHY_IS_CHAT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_CHAT))
#define EMPATHY_IS_CHAT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), EMPATHY_TYPE_CHAT))
#define EMPATHY_CHAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), EMPATHY_TYPE_CHAT, EmpathyChatClass))

typedef struct _EmpathyChat       EmpathyChat;
typedef struct _EmpathyChatClass  EmpathyChatClass;
typedef struct _EmpathyChatPriv   EmpathyChatPriv;

struct _EmpathyChat {
	GtkBox parent;
	EmpathyChatPriv *priv;

	/* Protected */
	EmpathyChatView *view;
	GtkWidget       *input_text_view;
};

struct _EmpathyChatClass {
	GtkBoxClass parent;
};

GType              empathy_chat_get_type             (void);
EmpathyChat *      empathy_chat_new                  (EmpathyTpChat *tp_chat);
EmpathyTpChat *    empathy_chat_get_tp_chat          (EmpathyChat   *chat);
void               empathy_chat_set_tp_chat          (EmpathyChat   *chat,
						      EmpathyTpChat *tp_chat);
TpAccount *        empathy_chat_get_account          (EmpathyChat   *chat);
const gchar *      empathy_chat_get_id               (EmpathyChat   *chat);
gchar *            empathy_chat_dup_name             (EmpathyChat   *chat);
const gchar *      empathy_chat_get_subject          (EmpathyChat   *chat);
EmpathyContact *   empathy_chat_get_remote_contact   (EmpathyChat   *chat);
GtkWidget *        empathy_chat_get_contact_menu     (EmpathyChat   *chat);
void               empathy_chat_clear                (EmpathyChat   *chat);
void               empathy_chat_scroll_down          (EmpathyChat   *chat);
void               empathy_chat_cut                  (EmpathyChat   *chat);
void               empathy_chat_copy                 (EmpathyChat   *chat);
void               empathy_chat_paste                (EmpathyChat   *chat);
void               empathy_chat_find                 (EmpathyChat   *chat);
void               empathy_chat_correct_word         (EmpathyChat   *chat,
						      GtkTextIter   *start,
						      GtkTextIter   *end,
						      const gchar   *new_word);
void               empathy_chat_join_muc             (EmpathyChat   *chat,
						      const gchar   *room);
gboolean           empathy_chat_is_room              (EmpathyChat   *chat);
void               empathy_chat_set_show_contacts    (EmpathyChat *chat,
                                                      gboolean     show);
guint              empathy_chat_get_nb_unread_messages (EmpathyChat   *chat);

void               empathy_chat_messages_read        (EmpathyChat *self);

gboolean           empathy_chat_is_composing (EmpathyChat *chat);

gboolean           empathy_chat_is_sms_channel       (EmpathyChat *self);
guint              empathy_chat_get_n_messages_sending (EmpathyChat *self);

gchar *            empathy_chat_dup_text             (EmpathyChat *self);
void               empathy_chat_set_text             (EmpathyChat *self,
                                                      const gchar *text);

G_END_DECLS

#endif /* __EMPATHY_CHAT_H__ */
