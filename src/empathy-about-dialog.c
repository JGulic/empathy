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

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <libempathy-gtk/empathy-ui-utils.h>

#include "empathy-about-dialog.h"

#define WEB_SITE "http://live.gnome.org/Empathy"

static const char *authors[] = {
	"Alban Crequy",
	"Andreas Lööw",
	"Aurelien Naldi",
	"Bastien Nocera",
	"Christoffer Olsen",
	"Cosimo Cecchi",
	"Danielle Madeley",
	"Elliot Fairweather",
	"Frederic Crozat",
	"Frederic Peters",
	"Geert-Jan Van den Bogaerde",
	"Guillaume Desmottes <guillaume.desmottes@collabora.co.uk>",
	"Johan Hammar",
	"Jonatan Magnusson",
	"Jonny Lamb",
	"Jordi Mallach",
	"Kim Andersen",
	"Marco Barisione",
	"Martyn Russell <martyn@gnome.org>",
	"Mikael Hallendal <micke@imendio.com>",
	"Mike Gratton",
	"Richard Hult <richard@imendio.com>",
	"Ross Burton",
	"Sjoerd Simons",
	"Thomas Reynolds",
	"Vincent Untz",
	"Xavier Claessens <xclaesse@gmail.com>",
	NULL
};

static const char *documenters[] = {
	"Milo Casagrande",
	"Seth Dudenhofer",
	NULL
};

static const char *artists[] = {
	"Andreas Nilsson <nisses.mail@home.se>",
	"Vinicius Depizzol <vdepizzol@gmail.com>",
	"K.Vishnoo Charan Reddy <drkvi-a@yahoo.com>",
	NULL
};

static const char *license[] = {
	N_("Empathy is free software; you can redistribute it and/or modify "
	   "it under the terms of the GNU General Public License as published by "
	   "the Free Software Foundation; either version 2 of the License, or "
	   "(at your option) any later version."),
	N_("Empathy is distributed in the hope that it will be useful, "
	   "but WITHOUT ANY WARRANTY; without even the implied warranty of "
	   "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
	   "GNU General Public License for more details."),
	N_("You should have received a copy of the GNU General Public License "
	   "along with Empathy; if not, write to the Free Software Foundation, Inc., "
	   "51 Franklin Street, Fifth Floor, Boston, MA 02110-130159 USA"),
	NULL
};

void
empathy_about_dialog_new (GtkWindow *parent)
{
	GString *license_trans = g_string_new (NULL);
	int i;

	for (i = 0; license[i] != NULL; i++) {
		g_string_append (license_trans, _(license[i]));
		g_string_append (license_trans, "\n\n");

	}
	gtk_show_about_dialog (parent,
			       "artists", artists,
			       "authors", authors,
			       "comments", _("An Instant Messaging client for GNOME"),
			       "license", license_trans->str,
			       "wrap-license", TRUE,
			       "copyright", "Imendio AB 2002-2007\nCollabora Ltd 2007-2011",
			       "documenters", documenters,
			       "logo-icon-name", "empathy",
			       "translator-credits", _("translator-credits"),
			       "version", PACKAGE_VERSION,
			       "website", WEB_SITE,
			       NULL);

	g_string_free (license_trans, TRUE);
}


