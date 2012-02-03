


#ifndef __LIBEMPATHY_GTK_ENUM_TYPES_H__
#define __LIBEMPATHY_GTK_ENUM_TYPES_H__ 1

#include <glib-object.h>

G_BEGIN_DECLS

#include <libempathy-gtk/empathy-contact-widget.h>
#define EMPATHY_TYPE_CONTACT_WIDGET_FLAGS empathy_contact_widget_flags_get_type()
GType empathy_contact_widget_flags_get_type (void);
#include <libempathy-gtk/empathy-individual-menu.h>
#define EMPATHY_TYPE_INDIVIDUAL_FEATURE_FLAGS empathy_individual_feature_flags_get_type()
GType empathy_individual_feature_flags_get_type (void);
#include <libempathy-gtk/empathy-individual-store.h>
#define EMPATHY_TYPE_INDIVIDUAL_STORE_SORT empathy_individual_store_sort_get_type()
GType empathy_individual_store_sort_get_type (void);
#define EMPATHY_TYPE_INDIVIDUAL_STORE_COL empathy_individual_store_col_get_type()
GType empathy_individual_store_col_get_type (void);
#include <libempathy-gtk/empathy-individual-view.h>
#define EMPATHY_TYPE_INDIVIDUAL_VIEW_FEATURE_FLAGS empathy_individual_view_feature_flags_get_type()
GType empathy_individual_view_feature_flags_get_type (void);
#include <libempathy-gtk/empathy-individual-widget.h>
#define EMPATHY_TYPE_INDIVIDUAL_WIDGET_FLAGS empathy_individual_widget_flags_get_type()
GType empathy_individual_widget_flags_get_type (void);
#include <libempathy-gtk/empathy-notify-manager.h>
#define EMPATHY_TYPE_NOTIFICATION_CLOSED_REASON empathy_notification_closed_reason_get_type()
GType empathy_notification_closed_reason_get_type (void);
#include <libempathy-gtk/empathy-persona-store.h>
#define EMPATHY_TYPE_PERSONA_STORE_SORT empathy_persona_store_sort_get_type()
GType empathy_persona_store_sort_get_type (void);
#define EMPATHY_TYPE_PERSONA_STORE_COL empathy_persona_store_col_get_type()
GType empathy_persona_store_col_get_type (void);
#include <libempathy-gtk/empathy-persona-view.h>
#define EMPATHY_TYPE_PERSONA_VIEW_FEATURE_FLAGS empathy_persona_view_feature_flags_get_type()
GType empathy_persona_view_feature_flags_get_type (void);
#include <libempathy-gtk/empathy-sound-manager.h>
#define EMPATHY_TYPE_SOUND empathy_sound_get_type()
GType empathy_sound_get_type (void);
#include <libempathy-gtk/empathy-webkit-utils.h>
#define EMPATHY_TYPE_WEB_KIT_MENU_FLAGS empathy_web_kit_menu_flags_get_type()
GType empathy_web_kit_menu_flags_get_type (void);
G_END_DECLS

#endif /* __LIBEMPATHY_GTK_ENUM_TYPES_H__ */



