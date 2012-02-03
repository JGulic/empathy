


#include <config.h>
#include <glib-object.h>
#include "empathy-gtk-enum-types.h"


/* enumerations from "empathy-contact-widget.h" */
static const GFlagsValue _empathy_contact_widget_flags_values[] = {
  { EMPATHY_CONTACT_WIDGET_EDIT_NONE, "EMPATHY_CONTACT_WIDGET_EDIT_NONE", "edit-none" },
  { EMPATHY_CONTACT_WIDGET_EDIT_ALIAS, "EMPATHY_CONTACT_WIDGET_EDIT_ALIAS", "edit-alias" },
  { EMPATHY_CONTACT_WIDGET_EDIT_AVATAR, "EMPATHY_CONTACT_WIDGET_EDIT_AVATAR", "edit-avatar" },
  { EMPATHY_CONTACT_WIDGET_EDIT_ACCOUNT, "EMPATHY_CONTACT_WIDGET_EDIT_ACCOUNT", "edit-account" },
  { EMPATHY_CONTACT_WIDGET_EDIT_ID, "EMPATHY_CONTACT_WIDGET_EDIT_ID", "edit-id" },
  { EMPATHY_CONTACT_WIDGET_EDIT_GROUPS, "EMPATHY_CONTACT_WIDGET_EDIT_GROUPS", "edit-groups" },
  { EMPATHY_CONTACT_WIDGET_FOR_TOOLTIP, "EMPATHY_CONTACT_WIDGET_FOR_TOOLTIP", "for-tooltip" },
  { EMPATHY_CONTACT_WIDGET_SHOW_LOCATION, "EMPATHY_CONTACT_WIDGET_SHOW_LOCATION", "show-location" },
  { EMPATHY_CONTACT_WIDGET_NO_SET_ALIAS, "EMPATHY_CONTACT_WIDGET_NO_SET_ALIAS", "no-set-alias" },
  { EMPATHY_CONTACT_WIDGET_SHOW_DETAILS, "EMPATHY_CONTACT_WIDGET_SHOW_DETAILS", "show-details" },
  { EMPATHY_CONTACT_WIDGET_EDIT_DETAILS, "EMPATHY_CONTACT_WIDGET_EDIT_DETAILS", "edit-details" },
  { EMPATHY_CONTACT_WIDGET_NO_STATUS, "EMPATHY_CONTACT_WIDGET_NO_STATUS", "no-status" },
  { 0, NULL, NULL }
};

GType
empathy_contact_widget_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyContactWidgetFlags", _empathy_contact_widget_flags_values);

  return type;
}


/* enumerations from "empathy-individual-menu.h" */
static const GFlagsValue _empathy_individual_feature_flags_values[] = {
  { EMPATHY_INDIVIDUAL_FEATURE_NONE, "EMPATHY_INDIVIDUAL_FEATURE_NONE", "none" },
  { EMPATHY_INDIVIDUAL_FEATURE_CHAT, "EMPATHY_INDIVIDUAL_FEATURE_CHAT", "chat" },
  { EMPATHY_INDIVIDUAL_FEATURE_CALL, "EMPATHY_INDIVIDUAL_FEATURE_CALL", "call" },
  { EMPATHY_INDIVIDUAL_FEATURE_LOG, "EMPATHY_INDIVIDUAL_FEATURE_LOG", "log" },
  { EMPATHY_INDIVIDUAL_FEATURE_EDIT, "EMPATHY_INDIVIDUAL_FEATURE_EDIT", "edit" },
  { EMPATHY_INDIVIDUAL_FEATURE_INFO, "EMPATHY_INDIVIDUAL_FEATURE_INFO", "info" },
  { EMPATHY_INDIVIDUAL_FEATURE_FAVOURITE, "EMPATHY_INDIVIDUAL_FEATURE_FAVOURITE", "favourite" },
  { EMPATHY_INDIVIDUAL_FEATURE_LINK, "EMPATHY_INDIVIDUAL_FEATURE_LINK", "link" },
  { EMPATHY_INDIVIDUAL_FEATURE_SMS, "EMPATHY_INDIVIDUAL_FEATURE_SMS", "sms" },
  { EMPATHY_INDIVIDUAL_FEATURE_CALL_PHONE, "EMPATHY_INDIVIDUAL_FEATURE_CALL_PHONE", "call-phone" },
  { EMPATHY_INDIVIDUAL_FEATURE_ADD_CONTACT, "EMPATHY_INDIVIDUAL_FEATURE_ADD_CONTACT", "add-contact" },
  { EMPATHY_INDIVIDUAL_FEATURE_BLOCK, "EMPATHY_INDIVIDUAL_FEATURE_BLOCK", "block" },
  { 0, NULL, NULL }
};

GType
empathy_individual_feature_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyIndividualFeatureFlags", _empathy_individual_feature_flags_values);

  return type;
}


/* enumerations from "empathy-individual-store.h" */
static const GEnumValue _empathy_individual_store_sort_values[] = {
  { EMPATHY_INDIVIDUAL_STORE_SORT_STATE, "EMPATHY_INDIVIDUAL_STORE_SORT_STATE", "state" },
  { EMPATHY_INDIVIDUAL_STORE_SORT_NAME, "EMPATHY_INDIVIDUAL_STORE_SORT_NAME", "name" },
  { 0, NULL, NULL }
};

GType
empathy_individual_store_sort_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyIndividualStoreSort", _empathy_individual_store_sort_values);

  return type;
}

static const GEnumValue _empathy_individual_store_col_values[] = {
  { EMPATHY_INDIVIDUAL_STORE_COL_ICON_STATUS, "EMPATHY_INDIVIDUAL_STORE_COL_ICON_STATUS", "icon-status" },
  { EMPATHY_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR, "EMPATHY_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR", "pixbuf-avatar" },
  { EMPATHY_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR_VISIBLE, "EMPATHY_INDIVIDUAL_STORE_COL_PIXBUF_AVATAR_VISIBLE", "pixbuf-avatar-visible" },
  { EMPATHY_INDIVIDUAL_STORE_COL_NAME, "EMPATHY_INDIVIDUAL_STORE_COL_NAME", "name" },
  { EMPATHY_INDIVIDUAL_STORE_COL_PRESENCE_TYPE, "EMPATHY_INDIVIDUAL_STORE_COL_PRESENCE_TYPE", "presence-type" },
  { EMPATHY_INDIVIDUAL_STORE_COL_STATUS, "EMPATHY_INDIVIDUAL_STORE_COL_STATUS", "status" },
  { EMPATHY_INDIVIDUAL_STORE_COL_COMPACT, "EMPATHY_INDIVIDUAL_STORE_COL_COMPACT", "compact" },
  { EMPATHY_INDIVIDUAL_STORE_COL_INDIVIDUAL, "EMPATHY_INDIVIDUAL_STORE_COL_INDIVIDUAL", "individual" },
  { EMPATHY_INDIVIDUAL_STORE_COL_IS_GROUP, "EMPATHY_INDIVIDUAL_STORE_COL_IS_GROUP", "is-group" },
  { EMPATHY_INDIVIDUAL_STORE_COL_IS_ACTIVE, "EMPATHY_INDIVIDUAL_STORE_COL_IS_ACTIVE", "is-active" },
  { EMPATHY_INDIVIDUAL_STORE_COL_IS_ONLINE, "EMPATHY_INDIVIDUAL_STORE_COL_IS_ONLINE", "is-online" },
  { EMPATHY_INDIVIDUAL_STORE_COL_IS_SEPARATOR, "EMPATHY_INDIVIDUAL_STORE_COL_IS_SEPARATOR", "is-separator" },
  { EMPATHY_INDIVIDUAL_STORE_COL_CAN_AUDIO_CALL, "EMPATHY_INDIVIDUAL_STORE_COL_CAN_AUDIO_CALL", "can-audio-call" },
  { EMPATHY_INDIVIDUAL_STORE_COL_CAN_VIDEO_CALL, "EMPATHY_INDIVIDUAL_STORE_COL_CAN_VIDEO_CALL", "can-video-call" },
  { EMPATHY_INDIVIDUAL_STORE_COL_IS_FAKE_GROUP, "EMPATHY_INDIVIDUAL_STORE_COL_IS_FAKE_GROUP", "is-fake-group" },
  { EMPATHY_INDIVIDUAL_STORE_COL_CLIENT_TYPES, "EMPATHY_INDIVIDUAL_STORE_COL_CLIENT_TYPES", "client-types" },
  { EMPATHY_INDIVIDUAL_STORE_COL_EVENT_COUNT, "EMPATHY_INDIVIDUAL_STORE_COL_EVENT_COUNT", "event-count" },
  { EMPATHY_INDIVIDUAL_STORE_COL_COUNT, "EMPATHY_INDIVIDUAL_STORE_COL_COUNT", "count" },
  { 0, NULL, NULL }
};

GType
empathy_individual_store_col_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyIndividualStoreCol", _empathy_individual_store_col_values);

  return type;
}


/* enumerations from "empathy-individual-view.h" */
static const GFlagsValue _empathy_individual_view_feature_flags_values[] = {
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_NONE, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_NONE", "none" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_SAVE", "groups-save" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_RENAME, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_RENAME", "groups-rename" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_REMOVE, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_REMOVE", "groups-remove" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_CHANGE, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_GROUPS_CHANGE", "groups-change" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_REMOVE, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_REMOVE", "individual-remove" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_DROP, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_DROP", "individual-drop" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_DRAG, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_DRAG", "individual-drag" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_TOOLTIP, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_TOOLTIP", "individual-tooltip" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_CALL, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_INDIVIDUAL_CALL", "individual-call" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_PERSONA_DROP, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_PERSONA_DROP", "persona-drop" },
  { EMPATHY_INDIVIDUAL_VIEW_FEATURE_FILE_DROP, "EMPATHY_INDIVIDUAL_VIEW_FEATURE_FILE_DROP", "file-drop" },
  { 0, NULL, NULL }
};

GType
empathy_individual_view_feature_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyIndividualViewFeatureFlags", _empathy_individual_view_feature_flags_values);

  return type;
}


/* enumerations from "empathy-individual-widget.h" */
static const GFlagsValue _empathy_individual_widget_flags_values[] = {
  { EMPATHY_INDIVIDUAL_WIDGET_NONE, "EMPATHY_INDIVIDUAL_WIDGET_NONE", "none" },
  { EMPATHY_INDIVIDUAL_WIDGET_EDIT_ALIAS, "EMPATHY_INDIVIDUAL_WIDGET_EDIT_ALIAS", "edit-alias" },
  { EMPATHY_INDIVIDUAL_WIDGET_EDIT_FAVOURITE, "EMPATHY_INDIVIDUAL_WIDGET_EDIT_FAVOURITE", "edit-favourite" },
  { EMPATHY_INDIVIDUAL_WIDGET_EDIT_GROUPS, "EMPATHY_INDIVIDUAL_WIDGET_EDIT_GROUPS", "edit-groups" },
  { EMPATHY_INDIVIDUAL_WIDGET_FOR_TOOLTIP, "EMPATHY_INDIVIDUAL_WIDGET_FOR_TOOLTIP", "for-tooltip" },
  { EMPATHY_INDIVIDUAL_WIDGET_SHOW_LOCATION, "EMPATHY_INDIVIDUAL_WIDGET_SHOW_LOCATION", "show-location" },
  { EMPATHY_INDIVIDUAL_WIDGET_SHOW_DETAILS, "EMPATHY_INDIVIDUAL_WIDGET_SHOW_DETAILS", "show-details" },
  { EMPATHY_INDIVIDUAL_WIDGET_SHOW_PERSONAS, "EMPATHY_INDIVIDUAL_WIDGET_SHOW_PERSONAS", "show-personas" },
  { EMPATHY_INDIVIDUAL_WIDGET_SHOW_CLIENT_TYPES, "EMPATHY_INDIVIDUAL_WIDGET_SHOW_CLIENT_TYPES", "show-client-types" },
  { 0, NULL, NULL }
};

GType
empathy_individual_widget_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyIndividualWidgetFlags", _empathy_individual_widget_flags_values);

  return type;
}


/* enumerations from "empathy-notify-manager.h" */
static const GEnumValue _empathy_notification_closed_reason_values[] = {
  { EMPATHY_NOTIFICATION_CLOSED_INVALID, "EMPATHY_NOTIFICATION_CLOSED_INVALID", "invalid" },
  { EMPATHY_NOTIFICATION_CLOSED_EXPIRED, "EMPATHY_NOTIFICATION_CLOSED_EXPIRED", "expired" },
  { EMPATHY_NOTIFICATION_CLOSED_DISMISSED, "EMPATHY_NOTIFICATION_CLOSED_DISMISSED", "dismissed" },
  { EMPATHY_NOTIFICATION_CLOSED_PROGRAMMATICALY, "EMPATHY_NOTIFICATION_CLOSED_PROGRAMMATICALY", "programmaticaly" },
  { EMPATHY_NOTIFICATION_CLOSED_RESERVED, "EMPATHY_NOTIFICATION_CLOSED_RESERVED", "reserved" },
  { 0, NULL, NULL }
};

GType
empathy_notification_closed_reason_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyNotificationClosedReason", _empathy_notification_closed_reason_values);

  return type;
}


/* enumerations from "empathy-persona-store.h" */
static const GEnumValue _empathy_persona_store_sort_values[] = {
  { EMPATHY_PERSONA_STORE_SORT_STATE, "EMPATHY_PERSONA_STORE_SORT_STATE", "state" },
  { EMPATHY_PERSONA_STORE_SORT_NAME, "EMPATHY_PERSONA_STORE_SORT_NAME", "name" },
  { 0, NULL, NULL }
};

GType
empathy_persona_store_sort_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyPersonaStoreSort", _empathy_persona_store_sort_values);

  return type;
}

static const GEnumValue _empathy_persona_store_col_values[] = {
  { EMPATHY_PERSONA_STORE_COL_ICON_STATUS, "EMPATHY_PERSONA_STORE_COL_ICON_STATUS", "icon-status" },
  { EMPATHY_PERSONA_STORE_COL_PIXBUF_AVATAR, "EMPATHY_PERSONA_STORE_COL_PIXBUF_AVATAR", "pixbuf-avatar" },
  { EMPATHY_PERSONA_STORE_COL_PIXBUF_AVATAR_VISIBLE, "EMPATHY_PERSONA_STORE_COL_PIXBUF_AVATAR_VISIBLE", "pixbuf-avatar-visible" },
  { EMPATHY_PERSONA_STORE_COL_NAME, "EMPATHY_PERSONA_STORE_COL_NAME", "name" },
  { EMPATHY_PERSONA_STORE_COL_ACCOUNT_NAME, "EMPATHY_PERSONA_STORE_COL_ACCOUNT_NAME", "account-name" },
  { EMPATHY_PERSONA_STORE_COL_DISPLAY_ID, "EMPATHY_PERSONA_STORE_COL_DISPLAY_ID", "display-id" },
  { EMPATHY_PERSONA_STORE_COL_PRESENCE_TYPE, "EMPATHY_PERSONA_STORE_COL_PRESENCE_TYPE", "presence-type" },
  { EMPATHY_PERSONA_STORE_COL_STATUS, "EMPATHY_PERSONA_STORE_COL_STATUS", "status" },
  { EMPATHY_PERSONA_STORE_COL_PERSONA, "EMPATHY_PERSONA_STORE_COL_PERSONA", "persona" },
  { EMPATHY_PERSONA_STORE_COL_IS_ACTIVE, "EMPATHY_PERSONA_STORE_COL_IS_ACTIVE", "is-active" },
  { EMPATHY_PERSONA_STORE_COL_IS_ONLINE, "EMPATHY_PERSONA_STORE_COL_IS_ONLINE", "is-online" },
  { EMPATHY_PERSONA_STORE_COL_CAN_AUDIO_CALL, "EMPATHY_PERSONA_STORE_COL_CAN_AUDIO_CALL", "can-audio-call" },
  { EMPATHY_PERSONA_STORE_COL_CAN_VIDEO_CALL, "EMPATHY_PERSONA_STORE_COL_CAN_VIDEO_CALL", "can-video-call" },
  { EMPATHY_PERSONA_STORE_COL_COUNT, "EMPATHY_PERSONA_STORE_COL_COUNT", "count" },
  { 0, NULL, NULL }
};

GType
empathy_persona_store_col_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyPersonaStoreCol", _empathy_persona_store_col_values);

  return type;
}


/* enumerations from "empathy-persona-view.h" */
static const GFlagsValue _empathy_persona_view_feature_flags_values[] = {
  { EMPATHY_PERSONA_VIEW_FEATURE_NONE, "EMPATHY_PERSONA_VIEW_FEATURE_NONE", "none" },
  { EMPATHY_PERSONA_VIEW_FEATURE_PERSONA_DRAG, "EMPATHY_PERSONA_VIEW_FEATURE_PERSONA_DRAG", "persona-drag" },
  { EMPATHY_PERSONA_VIEW_FEATURE_PERSONA_DROP, "EMPATHY_PERSONA_VIEW_FEATURE_PERSONA_DROP", "persona-drop" },
  { EMPATHY_PERSONA_VIEW_FEATURE_ALL, "EMPATHY_PERSONA_VIEW_FEATURE_ALL", "all" },
  { 0, NULL, NULL }
};

GType
empathy_persona_view_feature_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyPersonaViewFeatureFlags", _empathy_persona_view_feature_flags_values);

  return type;
}


/* enumerations from "empathy-sound-manager.h" */
static const GEnumValue _empathy_sound_values[] = {
  { EMPATHY_SOUND_MESSAGE_INCOMING, "EMPATHY_SOUND_MESSAGE_INCOMING", "empathy-sound-message-incoming" },
  { EMPATHY_SOUND_MESSAGE_OUTGOING, "EMPATHY_SOUND_MESSAGE_OUTGOING", "empathy-sound-message-outgoing" },
  { EMPATHY_SOUND_CONVERSATION_NEW, "EMPATHY_SOUND_CONVERSATION_NEW", "empathy-sound-conversation-new" },
  { EMPATHY_SOUND_CONTACT_CONNECTED, "EMPATHY_SOUND_CONTACT_CONNECTED", "empathy-sound-contact-connected" },
  { EMPATHY_SOUND_CONTACT_DISCONNECTED, "EMPATHY_SOUND_CONTACT_DISCONNECTED", "empathy-sound-contact-disconnected" },
  { EMPATHY_SOUND_ACCOUNT_CONNECTED, "EMPATHY_SOUND_ACCOUNT_CONNECTED", "empathy-sound-account-connected" },
  { EMPATHY_SOUND_ACCOUNT_DISCONNECTED, "EMPATHY_SOUND_ACCOUNT_DISCONNECTED", "empathy-sound-account-disconnected" },
  { EMPATHY_SOUND_PHONE_INCOMING, "EMPATHY_SOUND_PHONE_INCOMING", "empathy-sound-phone-incoming" },
  { EMPATHY_SOUND_PHONE_OUTGOING, "EMPATHY_SOUND_PHONE_OUTGOING", "empathy-sound-phone-outgoing" },
  { EMPATHY_SOUND_PHONE_HANGUP, "EMPATHY_SOUND_PHONE_HANGUP", "empathy-sound-phone-hangup" },
  { LAST_EMPATHY_SOUND, "LAST_EMPATHY_SOUND", "last-empathy-sound" },
  { 0, NULL, NULL }
};

GType
empathy_sound_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathySound", _empathy_sound_values);

  return type;
}


/* enumerations from "empathy-webkit-utils.h" */
static const GFlagsValue _empathy_web_kit_menu_flags_values[] = {
  { EMPATHY_WEBKIT_MENU_CLEAR, "EMPATHY_WEBKIT_MENU_CLEAR", "clear" },
  { 0, NULL, NULL }
};

GType
empathy_web_kit_menu_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyWebKitMenuFlags", _empathy_web_kit_menu_flags_values);

  return type;
}




