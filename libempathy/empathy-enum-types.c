


#include <config.h>
#include <glib-object.h>
#include "empathy-enum-types.h"


/* enumerations from "empathy-contact.h" */
static const GFlagsValue _empathy_capabilities_values[] = {
  { EMPATHY_CAPABILITIES_NONE, "EMPATHY_CAPABILITIES_NONE", "none" },
  { EMPATHY_CAPABILITIES_AUDIO, "EMPATHY_CAPABILITIES_AUDIO", "audio" },
  { EMPATHY_CAPABILITIES_VIDEO, "EMPATHY_CAPABILITIES_VIDEO", "video" },
  { EMPATHY_CAPABILITIES_FT, "EMPATHY_CAPABILITIES_FT", "ft" },
  { EMPATHY_CAPABILITIES_RFB_STREAM_TUBE, "EMPATHY_CAPABILITIES_RFB_STREAM_TUBE", "rfb-stream-tube" },
  { EMPATHY_CAPABILITIES_SMS, "EMPATHY_CAPABILITIES_SMS", "sms" },
  { EMPATHY_CAPABILITIES_UNKNOWN, "EMPATHY_CAPABILITIES_UNKNOWN", "unknown" },
  { 0, NULL, NULL }
};

GType
empathy_capabilities_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyCapabilities", _empathy_capabilities_values);

  return type;
}

static const GEnumValue _empathy_action_type_values[] = {
  { EMPATHY_ACTION_CHAT, "EMPATHY_ACTION_CHAT", "chat" },
  { EMPATHY_ACTION_SMS, "EMPATHY_ACTION_SMS", "sms" },
  { EMPATHY_ACTION_AUDIO_CALL, "EMPATHY_ACTION_AUDIO_CALL", "audio-call" },
  { EMPATHY_ACTION_VIDEO_CALL, "EMPATHY_ACTION_VIDEO_CALL", "video-call" },
  { EMPATHY_ACTION_VIEW_LOGS, "EMPATHY_ACTION_VIEW_LOGS", "view-logs" },
  { EMPATHY_ACTION_SEND_FILE, "EMPATHY_ACTION_SEND_FILE", "send-file" },
  { EMPATHY_ACTION_SHARE_MY_DESKTOP, "EMPATHY_ACTION_SHARE_MY_DESKTOP", "share-my-desktop" },
  { 0, NULL, NULL }
};

GType
empathy_action_type_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyActionType", _empathy_action_type_values);

  return type;
}


/* enumerations from "empathy-debug.h" */
static const GFlagsValue _empathy_debug_flags_values[] = {
  { EMPATHY_DEBUG_TP, "EMPATHY_DEBUG_TP", "tp" },
  { EMPATHY_DEBUG_CHAT, "EMPATHY_DEBUG_CHAT", "chat" },
  { EMPATHY_DEBUG_CONTACT, "EMPATHY_DEBUG_CONTACT", "contact" },
  { EMPATHY_DEBUG_ACCOUNT, "EMPATHY_DEBUG_ACCOUNT", "account" },
  { EMPATHY_DEBUG_IRC, "EMPATHY_DEBUG_IRC", "irc" },
  { EMPATHY_DEBUG_DISPATCHER, "EMPATHY_DEBUG_DISPATCHER", "dispatcher" },
  { EMPATHY_DEBUG_FT, "EMPATHY_DEBUG_FT", "ft" },
  { EMPATHY_DEBUG_LOCATION, "EMPATHY_DEBUG_LOCATION", "location" },
  { EMPATHY_DEBUG_OTHER, "EMPATHY_DEBUG_OTHER", "other" },
  { EMPATHY_DEBUG_SHARE_DESKTOP, "EMPATHY_DEBUG_SHARE_DESKTOP", "share-desktop" },
  { EMPATHY_DEBUG_CONNECTIVITY, "EMPATHY_DEBUG_CONNECTIVITY", "connectivity" },
  { EMPATHY_DEBUG_IMPORT_MC4_ACCOUNTS, "EMPATHY_DEBUG_IMPORT_MC4_ACCOUNTS", "import-mc4-accounts" },
  { EMPATHY_DEBUG_TESTS, "EMPATHY_DEBUG_TESTS", "tests" },
  { EMPATHY_DEBUG_VOIP, "EMPATHY_DEBUG_VOIP", "voip" },
  { EMPATHY_DEBUG_TLS, "EMPATHY_DEBUG_TLS", "tls" },
  { EMPATHY_DEBUG_SASL, "EMPATHY_DEBUG_SASL", "sasl" },
  { 0, NULL, NULL }
};

GType
empathy_debug_flags_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_flags_register_static ("EmpathyDebugFlags", _empathy_debug_flags_values);

  return type;
}


/* enumerations from "empathy-ft-handler.h" */
static const GEnumValue _empathy_ft_error_enum_values[] = {
  { EMPATHY_FT_ERROR_FAILED, "EMPATHY_FT_ERROR_FAILED", "failed" },
  { EMPATHY_FT_ERROR_HASH_MISMATCH, "EMPATHY_FT_ERROR_HASH_MISMATCH", "hash-mismatch" },
  { EMPATHY_FT_ERROR_TP_ERROR, "EMPATHY_FT_ERROR_TP_ERROR", "tp-error" },
  { EMPATHY_FT_ERROR_SOCKET, "EMPATHY_FT_ERROR_SOCKET", "socket" },
  { EMPATHY_FT_ERROR_NOT_SUPPORTED, "EMPATHY_FT_ERROR_NOT_SUPPORTED", "not-supported" },
  { EMPATHY_FT_ERROR_INVALID_SOURCE_FILE, "EMPATHY_FT_ERROR_INVALID_SOURCE_FILE", "invalid-source-file" },
  { EMPATHY_FT_ERROR_EMPTY_SOURCE_FILE, "EMPATHY_FT_ERROR_EMPTY_SOURCE_FILE", "empty-source-file" },
  { 0, NULL, NULL }
};

GType
empathy_ft_error_enum_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyFTErrorEnum", _empathy_ft_error_enum_values);

  return type;
}


/* enumerations from "empathy-tp-chat.h" */
static const GEnumValue _empathy_delivery_status_values[] = {
  { EMPATHY_DELIVERY_STATUS_NONE, "EMPATHY_DELIVERY_STATUS_NONE", "none" },
  { EMPATHY_DELIVERY_STATUS_SENDING, "EMPATHY_DELIVERY_STATUS_SENDING", "sending" },
  { EMPATHY_DELIVERY_STATUS_ACCEPTED, "EMPATHY_DELIVERY_STATUS_ACCEPTED", "accepted" },
  { 0, NULL, NULL }
};

GType
empathy_delivery_status_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyDeliveryStatus", _empathy_delivery_status_values);

  return type;
}


/* enumerations from "empathy-tp-streamed-media.h" */
static const GEnumValue _empathy_tp_streamed_media_status_values[] = {
  { EMPATHY_TP_STREAMED_MEDIA_STATUS_READYING, "EMPATHY_TP_STREAMED_MEDIA_STATUS_READYING", "readying" },
  { EMPATHY_TP_STREAMED_MEDIA_STATUS_PENDING, "EMPATHY_TP_STREAMED_MEDIA_STATUS_PENDING", "pending" },
  { EMPATHY_TP_STREAMED_MEDIA_STATUS_ACCEPTED, "EMPATHY_TP_STREAMED_MEDIA_STATUS_ACCEPTED", "accepted" },
  { EMPATHY_TP_STREAMED_MEDIA_STATUS_CLOSED, "EMPATHY_TP_STREAMED_MEDIA_STATUS_CLOSED", "closed" },
  { 0, NULL, NULL }
};

GType
empathy_tp_streamed_media_status_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_enum_register_static ("EmpathyTpStreamedMediaStatus", _empathy_tp_streamed_media_status_values);

  return type;
}




