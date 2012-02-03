/*
 * empathy-rounded-texture.h - Header for EmpathyRoundedTexture
 * Copyright (C) 2011 Collabora Ltd.
 * @author Emilio Pozuelo Monfort <emilio.pozuelo@collabora.co.uk>
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

#ifndef __EMPATHY_ROUNDED_TEXTURE_H__
#define __EMPATHY_ROUNDED_TEXTURE_H__

#include <glib-object.h>
#include <clutter/clutter.h>

G_BEGIN_DECLS

typedef struct _EmpathyRoundedTexture EmpathyRoundedTexture;
typedef struct _EmpathyRoundedTextureClass EmpathyRoundedTextureClass;

struct _EmpathyRoundedTextureClass {
    ClutterTextureClass parent_class;
};

struct _EmpathyRoundedTexture {
    ClutterTexture parent;
};

GType empathy_rounded_texture_get_type (void);

/* TYPE MACROS */
#define EMPATHY_TYPE_ROUNDED_TEXTURE \
  (empathy_rounded_texture_get_type ())
#define EMPATHY_ROUNDED_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EMPATHY_TYPE_ROUNDED_TEXTURE, \
    EmpathyRoundedTexture))
#define EMPATHY_ROUNDED_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), EMPATHY_TYPE_ROUNDED_TEXTURE, \
    EmpathyRoundedTextureClass))
#define EMPATHY_IS_ROUNDED_TEXTURE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EMPATHY_TYPE_ROUNDED_TEXTURE))
#define EMPATHY_IS_ROUNDED_TEXTURE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EMPATHY_TYPE_ROUNDED_TEXTURE))
#define EMPATHY_ROUNDED_TEXTURE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), EMPATHY_TYPE_ROUNDED_TEXTURE, \
    EmpathyRoundedTextureClass))

ClutterActor *empathy_rounded_texture_new (void);

G_END_DECLS

#endif /* #ifndef __EMPATHY_ROUNDED_TEXTURE_H__*/
