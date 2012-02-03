/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2008 Collabora Ltd.
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
 * Authors: Xavier Claessens <xclaesse@gmail.com>
 */

#ifndef __EMPATHY_CHAT_VIEW_H__
#define __EMPATHY_CHAT_VIEW_H__

#include <gtk/gtk.h>

#include <libempathy/empathy-contact.h>
#include <libempathy/empathy-message.h>

G_BEGIN_DECLS

#define EMPATHY_TYPE_CHAT_VIEW         (empathy_chat_view_get_type ())
#define EMPATHY_CHAT_VIEW(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), EMPATHY_TYPE_CHAT_VIEW, EmpathyChatView))
#define EMPATHY_IS_CHAT_VIEW(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EMPATHY_TYPE_CHAT_VIEW))
#define EMPATHY_TYPE_CHAT_VIEW_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), EMPATHY_TYPE_CHAT_VIEW, EmpathyChatViewIface))

typedef struct _EmpathyChatView      EmpathyChatView;
typedef struct _EmpathyChatViewIface EmpathyChatViewIface;

struct _EmpathyChatViewIface {
	GTypeInterface   base_iface;

	/* VTabled */
	void             (*append_message)       (EmpathyChatView *view,
						  EmpathyMessage  *msg);
	void             (*append_event)         (EmpathyChatView *view,
						  const gchar     *str);
	void             (*append_event_markup)  (EmpathyChatView *view,
						  const gchar     *markup_text,
						  const gchar     *fallback_text);
	void             (*edit_message)         (EmpathyChatView *view,
						  EmpathyMessage  *message);
	void             (*scroll)               (EmpathyChatView *view,
						  gboolean         allow_scrolling);
	void             (*scroll_down)          (EmpathyChatView *view);
	gboolean         (*get_has_selection)    (EmpathyChatView *view);
	void             (*clear)                (EmpathyChatView *view);
	gboolean         (*find_previous)        (EmpathyChatView *view,
						  const gchar     *search_criteria,
						  gboolean         new_search,
						  gboolean         match_case);
	gboolean         (*find_next)            (EmpathyChatView *view,
						  const gchar     *search_criteria,
						  gboolean         new_search,
						  gboolean         match_case);
	void             (*find_abilities)       (EmpathyChatView *view,
						  const gchar     *search_criteria,
						  gboolean         match_case,
						  gboolean        *can_do_previous,
						  gboolean        *can_do_next);
	void             (*highlight)            (EmpathyChatView *view,
						  const gchar     *text,
						  gboolean         match_case);
	void             (*copy_clipboard)       (EmpathyChatView *view);
	void             (*focus_toggled)        (EmpathyChatView *view,
						  gboolean         has_focus);
	void             (*message_acknowledged) (EmpathyChatView *view,
						  EmpathyMessage  *message);
};

GType            empathy_chat_view_get_type             (void) G_GNUC_CONST;
void             empathy_chat_view_append_message       (EmpathyChatView *view,
							 EmpathyMessage  *msg);
void             empathy_chat_view_append_event         (EmpathyChatView *view,
							 const gchar     *str);
void             empathy_chat_view_append_event_markup  (EmpathyChatView *view,
							 const gchar     *markup_text,
							 const gchar     *fallback_text);
void             empathy_chat_view_edit_message         (EmpathyChatView *view,
							 EmpathyMessage  *message);
void             empathy_chat_view_scroll               (EmpathyChatView *view,
							 gboolean         allow_scrolling);
void             empathy_chat_view_scroll_down          (EmpathyChatView *view);
gboolean         empathy_chat_view_get_has_selection    (EmpathyChatView *view);
void             empathy_chat_view_clear                (EmpathyChatView *view);
gboolean         empathy_chat_view_find_previous        (EmpathyChatView *view,
							 const gchar     *search_criteria,
							 gboolean         new_search,
							 gboolean         match_case);
gboolean         empathy_chat_view_find_next            (EmpathyChatView *view,
							 const gchar     *search_criteria,
							 gboolean         new_search,
							 gboolean         match_case);
void             empathy_chat_view_find_abilities       (EmpathyChatView *view,
							 const gchar     *search_criteria,
							 gboolean         match_case,
							 gboolean        *can_do_previous,
							 gboolean        *can_do_next);
void             empathy_chat_view_highlight            (EmpathyChatView *view,
							 const gchar     *text,
							 gboolean         match_case);
void             empathy_chat_view_copy_clipboard       (EmpathyChatView *view);
void             empathy_chat_view_focus_toggled        (EmpathyChatView *view,
							 gboolean         has_focus);
void             empathy_chat_view_message_acknowledged (EmpathyChatView *view,
							 EmpathyMessage  *message);

G_END_DECLS

#endif /* __EMPATHY_CHAT_VIEW_H__ */

