/*
 * Copyright (C) 2004 Imendio AB
 * Copyright (C) 2007-2012 Collabora Ltd.
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

#ifndef __EMPATHY_TIME_H__
#define __EMPATHY_TIME_H__

#ifndef __USE_XOPEN
#define __USE_XOPEN
#endif
#include <time.h>

#include <glib.h>

G_BEGIN_DECLS

/* FIXME: ideally we should only display the hour and minutes but
 * there is no localized format for that (bgo #668323) */
#define EMPATHY_TIME_FORMAT_DISPLAY_SHORT "%X"
#define EMPATHY_DATE_FORMAT_DISPLAY_SHORT "%a %d %b %Y"
#define EMPATHY_TIME_DATE_FORMAT_DISPLAY_SHORT "%a %d %b %Y, %X"

gint64  empathy_time_get_current     (void);
gchar  *empathy_time_to_string_utc   (gint64 t,
    const gchar *format);
gchar  *empathy_time_to_string_local (gint64 t,
    const gchar *format);
gchar  *empathy_time_to_string_relative (gint64 t);
gchar *empathy_duration_to_string (guint seconds);

G_END_DECLS

#endif /* __EMPATHY_TIME_H__ */

