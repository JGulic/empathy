/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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
 *          Travis Reitter <travis.reitter@collabora.co.uk>
 */

#ifndef __EMPATHY_INDIVIDUAL_DIALOGS_H__
#define __EMPATHY_INDIVIDUAL_DIALOGS_H__

#include <gtk/gtk.h>

#include <folks/folks.h>

G_BEGIN_DECLS

void empathy_new_individual_dialog_show (GtkWindow *parent);
void empathy_new_individual_dialog_show_with_individual (GtkWindow *parent,
    FolksIndividual *individual);
gboolean empathy_block_individual_dialog_show (GtkWindow *parent,
    FolksIndividual *individual,
    GdkPixbuf *avatar,
    gboolean *abusive);

G_END_DECLS

#endif /*  __EMPATHY_INDIVIDUAL_DIALOGS_H__ */
