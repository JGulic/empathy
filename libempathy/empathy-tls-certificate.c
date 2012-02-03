/*
 * empathy-tls-certificate.c - Source for EmpathyTLSCertificate
 * Copyright (C) 2010 Collabora Ltd.
 * @author Cosimo Cecchi <cosimo.cecchi@collabora.co.uk>
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

#include <config.h>

#include "empathy-tls-certificate.h"

#include <errno.h>

#include <glib/gstdio.h>

#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/util.h>

#define DEBUG_FLAG EMPATHY_DEBUG_TLS
#include "empathy-debug.h"
#include "empathy-utils.h"

#include "extensions/extensions.h"

enum {
  /* proxy properties */
  PROP_CERT_TYPE = 1,
  PROP_CERT_DATA,
  PROP_STATE,
  LAST_PROPERTY,
};

typedef struct {
  GSimpleAsyncResult *async_prepare_res;

  /* TLSCertificate properties */
  gchar *cert_type;
  GPtrArray *cert_data;
  EmpTLSCertificateState state;

  gboolean is_prepared;
} EmpathyTLSCertificatePriv;

G_DEFINE_TYPE (EmpathyTLSCertificate, empathy_tls_certificate,
    TP_TYPE_PROXY);

#define GET_PRIV(obj) EMPATHY_GET_PRIV (obj, EmpathyTLSCertificate);

static void
tls_certificate_got_all_cb (TpProxy *proxy,
    GHashTable *properties,
    const GError *error,
    gpointer user_data,
    GObject *weak_object)
{
  GPtrArray *cert_data;
  EmpathyTLSCertificate *self = EMPATHY_TLS_CERTIFICATE (weak_object);
  EmpathyTLSCertificatePriv *priv = GET_PRIV (self);

  if (error != NULL)
    {
      g_simple_async_result_set_from_error (priv->async_prepare_res, error);
      g_simple_async_result_complete (priv->async_prepare_res);
      tp_clear_object (&priv->async_prepare_res);

      return;
    }

  priv->cert_type = g_strdup (tp_asv_get_string (properties,
          "CertificateType"));
  priv->state = tp_asv_get_uint32 (properties, "State", NULL);

  cert_data = tp_asv_get_boxed (properties, "CertificateChainData",
      TP_ARRAY_TYPE_UCHAR_ARRAY_LIST);
  g_assert (cert_data != NULL);
  priv->cert_data = g_boxed_copy (TP_ARRAY_TYPE_UCHAR_ARRAY_LIST, cert_data);

  DEBUG ("Got a certificate chain long %u, of type %s",
      priv->cert_data->len, priv->cert_type);

  priv->is_prepared = TRUE;

  g_simple_async_result_complete (priv->async_prepare_res);
  tp_clear_object (&priv->async_prepare_res);
}

void
empathy_tls_certificate_prepare_async (EmpathyTLSCertificate *self,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  EmpathyTLSCertificatePriv *priv = GET_PRIV (self);

  /* emit an error if we're already preparing the object */
  if (priv->async_prepare_res != NULL)
    {
      g_simple_async_report_error_in_idle (G_OBJECT (self),
          callback, user_data,
          G_IO_ERROR, G_IO_ERROR_PENDING,
          "%s",
          "Prepare operation already in progress on the TLS certificate.");

      return;
    }

  /* if the object is already prepared, just complete in idle */
  if (priv->is_prepared)
    {
      tp_simple_async_report_success_in_idle (G_OBJECT (self),
          callback, user_data, empathy_tls_certificate_prepare_async);

      return;
    }

  priv->async_prepare_res = g_simple_async_result_new (G_OBJECT (self),
      callback, user_data, empathy_tls_certificate_prepare_async);

  /* call GetAll() on the certificate */
  tp_cli_dbus_properties_call_get_all (self,
      -1, EMP_IFACE_AUTHENTICATION_TLS_CERTIFICATE,
      tls_certificate_got_all_cb, NULL, NULL,
      G_OBJECT (self));
}

gboolean
empathy_tls_certificate_prepare_finish (EmpathyTLSCertificate *self,
    GAsyncResult *result,
    GError **error)
{
  gboolean retval = TRUE;

  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
          error))
    retval = FALSE;

  return retval;
}

static void
empathy_tls_certificate_finalize (GObject *object)
{
  EmpathyTLSCertificatePriv *priv = GET_PRIV (object);

  DEBUG ("%p", object);

  g_free (priv->cert_type);
  tp_clear_boxed (TP_ARRAY_TYPE_UCHAR_ARRAY_LIST, &priv->cert_data);

  G_OBJECT_CLASS (empathy_tls_certificate_parent_class)->finalize (object);
}

static void
empathy_tls_certificate_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  EmpathyTLSCertificatePriv *priv = GET_PRIV (object);

  switch (property_id)
    {
    case PROP_CERT_TYPE:
      g_value_set_string (value, priv->cert_type);
      break;
    case PROP_CERT_DATA:
      g_value_set_boxed (value, priv->cert_data);
      break;
    case PROP_STATE:
      g_value_set_uint (value, priv->state);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
empathy_tls_certificate_init (EmpathyTLSCertificate *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
      EMPATHY_TYPE_TLS_CERTIFICATE, EmpathyTLSCertificatePriv);
}

static void
empathy_tls_certificate_class_init (EmpathyTLSCertificateClass *klass)
{
  GParamSpec *pspec;
  GObjectClass *oclass = G_OBJECT_CLASS (klass);
  TpProxyClass *pclass = TP_PROXY_CLASS (klass);

  oclass->get_property = empathy_tls_certificate_get_property;
  oclass->finalize = empathy_tls_certificate_finalize;

  pclass->interface = EMP_IFACE_QUARK_AUTHENTICATION_TLS_CERTIFICATE;
  pclass->must_have_unique_name = TRUE;

  g_type_class_add_private (klass, sizeof (EmpathyTLSCertificatePriv));

  pspec = g_param_spec_string ("cert-type", "Certificate type",
      "The type of this certificate.",
      NULL,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_CERT_TYPE, pspec);

  pspec = g_param_spec_boxed ("cert-data", "Certificate chain data",
      "The raw DER-encoded certificate chain data.",
      TP_ARRAY_TYPE_UCHAR_ARRAY_LIST,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_CERT_DATA, pspec);

  pspec = g_param_spec_uint ("state", "State",
      "The state of this certificate.",
      EMP_TLS_CERTIFICATE_STATE_PENDING, NUM_EMP_TLS_CERTIFICATE_STATES -1,
      EMP_TLS_CERTIFICATE_STATE_PENDING,
      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  g_object_class_install_property (oclass, PROP_STATE, pspec);
}

static void
cert_proxy_accept_cb (TpProxy *proxy,
    const GError *error,
    gpointer user_data,
    GObject *weak_object)
{
  GSimpleAsyncResult *accept_result = user_data;

  DEBUG ("Callback for accept(), error %p", error);

  if (error != NULL)
    {
      DEBUG ("Error was %s", error->message);
      g_simple_async_result_set_from_error (accept_result, error);
    }

  g_simple_async_result_complete (accept_result);
}

static void
cert_proxy_reject_cb (TpProxy *proxy,
    const GError *error,
    gpointer user_data,
    GObject *weak_object)
{
  GSimpleAsyncResult *reject_result = user_data;

  DEBUG ("Callback for reject(), error %p", error);

  if (error != NULL)
    {
      DEBUG ("Error was %s", error->message);
      g_simple_async_result_set_from_error (reject_result, error);
    }

  g_simple_async_result_complete (reject_result);
}

static const gchar *
reject_reason_get_dbus_error (EmpTLSCertificateRejectReason reason)
{
  const gchar *retval = NULL;

  switch (reason)
    {
    case EMP_TLS_CERTIFICATE_REJECT_REASON_UNTRUSTED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_UNTRUSTED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_EXPIRED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_EXPIRED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_NOT_ACTIVATED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_NOT_ACTIVATED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_FINGERPRINT_MISMATCH:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_FINGERPRINT_MISMATCH);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_HOSTNAME_MISMATCH:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_HOSTNAME_MISMATCH);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_SELF_SIGNED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_SELF_SIGNED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_REVOKED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_REVOKED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_INSECURE:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_INSECURE);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_LIMIT_EXCEEDED:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_LIMIT_EXCEEDED);
      break;
    case EMP_TLS_CERTIFICATE_REJECT_REASON_UNKNOWN:
    default:
      retval = tp_error_get_dbus_name (TP_ERROR_CERT_INVALID);
      break;
    }

  return retval;
}

EmpathyTLSCertificate *
empathy_tls_certificate_new (TpDBusDaemon *dbus,
    const gchar *bus_name,
    const gchar *object_path,
    GError **error)
{
  EmpathyTLSCertificate *retval = NULL;

  if (!tp_dbus_check_valid_bus_name (bus_name,
          TP_DBUS_NAME_TYPE_UNIQUE, error))
    goto finally;

  if (!tp_dbus_check_valid_object_path (object_path, error))
    goto finally;

  retval = g_object_new (EMPATHY_TYPE_TLS_CERTIFICATE,
      "dbus-daemon", dbus,
      "bus-name", bus_name,
      "object-path", object_path,
      NULL);

finally:
  if (*error != NULL)
    DEBUG ("Error while creating the TLS certificate: %s",
        (*error)->message);

  return retval;
}

void
empathy_tls_certificate_accept_async (EmpathyTLSCertificate *self,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GSimpleAsyncResult *accept_result;

  g_assert (EMPATHY_IS_TLS_CERTIFICATE (self));

  DEBUG ("Accepting TLS certificate");

  accept_result = g_simple_async_result_new (G_OBJECT (self),
      callback, user_data, empathy_tls_certificate_accept_async);

  emp_cli_authentication_tls_certificate_call_accept (TP_PROXY (self),
      -1, cert_proxy_accept_cb,
      accept_result, g_object_unref,
      G_OBJECT (self));
}

gboolean
empathy_tls_certificate_accept_finish (EmpathyTLSCertificate *self,
    GAsyncResult *result,
    GError **error)
{
  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
          error))
    return FALSE;

  return TRUE;
}

static GPtrArray *
build_rejections_array (EmpTLSCertificateRejectReason reason,
    GHashTable *details)
{
  GPtrArray *retval;
  GValueArray *rejection;

  retval = g_ptr_array_new ();
  rejection = tp_value_array_build (3,
      G_TYPE_UINT, reason,
      G_TYPE_STRING, reject_reason_get_dbus_error (reason),
      TP_HASH_TYPE_STRING_VARIANT_MAP, details,
      NULL);

  g_ptr_array_add (retval, rejection);

  return retval;
}

void
empathy_tls_certificate_reject_async (EmpathyTLSCertificate *self,
    EmpTLSCertificateRejectReason reason,
    GHashTable *details,
    GAsyncReadyCallback callback,
    gpointer user_data)
{
  GPtrArray *rejections;
  GSimpleAsyncResult *reject_result;

  g_assert (EMPATHY_IS_TLS_CERTIFICATE (self));

  DEBUG ("Rejecting TLS certificate with reason %u", reason);

  rejections = build_rejections_array (reason, details);
  reject_result = g_simple_async_result_new (G_OBJECT (self),
      callback, user_data, empathy_tls_certificate_reject_async);

  emp_cli_authentication_tls_certificate_call_reject (TP_PROXY (self),
      -1, rejections, cert_proxy_reject_cb,
      reject_result, g_object_unref, G_OBJECT (self));

  tp_clear_boxed (EMP_ARRAY_TYPE_TLS_CERTIFICATE_REJECTION_LIST,
      &rejections);
}

gboolean
empathy_tls_certificate_reject_finish (EmpathyTLSCertificate *self,
    GAsyncResult *result,
    GError **error)
{
  if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result),
          error))
    return FALSE;

  return TRUE;
}
