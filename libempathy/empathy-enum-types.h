


#ifndef __LIBEMPATHY_ENUM_TYPES_H__
#define __LIBEMPATHY_ENUM_TYPES_H__ 1

#include <glib-object.h>

G_BEGIN_DECLS

#include <libempathy/empathy-contact.h>
#define EMPATHY_TYPE_CAPABILITIES empathy_capabilities_get_type()
GType empathy_capabilities_get_type (void);
#define EMPATHY_TYPE_ACTION_TYPE empathy_action_type_get_type()
GType empathy_action_type_get_type (void);
#include <libempathy/empathy-debug.h>
#define EMPATHY_TYPE_DEBUG_FLAGS empathy_debug_flags_get_type()
GType empathy_debug_flags_get_type (void);
#include <libempathy/empathy-ft-handler.h>
#define EMPATHY_TYPE_FT_ERROR_ENUM empathy_ft_error_enum_get_type()
GType empathy_ft_error_enum_get_type (void);
#include <libempathy/empathy-tp-chat.h>
#define EMPATHY_TYPE_DELIVERY_STATUS empathy_delivery_status_get_type()
GType empathy_delivery_status_get_type (void);
#include <libempathy/empathy-tp-streamed-media.h>
#define EMPATHY_TYPE_TP_STREAMED_MEDIA_STATUS empathy_tp_streamed_media_status_get_type()
GType empathy_tp_streamed_media_status_get_type (void);
G_END_DECLS

#endif /* __LIBEMPATHY_ENUM_TYPES_H__ */



