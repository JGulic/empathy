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
 * Authors: Jonny Lamb <jonny.lamb@collabora.co.uk>
 * */

#include <config.h>

#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <dbus/dbus-protocol.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <telepathy-glib/util.h>
#include <telepathy-glib/dbus.h>

#include "empathy-import-utils.h"
#include "empathy-import-pidgin.h"

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-utils.h>

#include <libempathy-gtk/empathy-ui-utils.h>

/* Pidgin to CM map */
typedef struct
{
  const gchar *protocol;
  const gchar *pidgin_name;
  const gchar *cm_name;
} PidginCmMapItem;

static PidginCmMapItem pidgin_cm_map[] =
{
  { "msn", "server", "server" },
  { "msn", "port", "port" },

  { "jabber", "connect_server", "server" },
  { "jabber", "port", "port" },
  { "jabber", "require_tls", "require-encryption" },
  { "jabber", "old_ssl", "old-ssl" },

  { "aim", "server", "server" },
  { "aim", "port", "port" },

  { "salut", "first", "first-name" },
  { "salut", "last", "last-name" },
  { "salut", "jid", "jid" },
  { "salut", "email", "email" },

  { "groupwise", "server", "server" },
  { "groupwise", "port", "port" },

  { "icq", "server", "server" },
  { "icq", "port", "port" },

  { "irc", "realname", "fullname" },
  { "irc", "ssl", "use-ssl" },
  { "irc", "port", "port" },

  { "yahoo", "server", "server" },
  { "yahoo", "port", "port" },
  { "yahoo", "xfer_port", "xfer-port" },
  { "yahoo", "ignore_invites", "ignore-invites" },
  { "yahoo", "yahoojp", "yahoojp" },
  { "yahoo", "xferjp_host", "xferjp-host" },
  { "yahoo", "serverjp", "serverjp" },
  { "yahoo", "xfer_host", "xfer-host" },
};

#define PIDGIN_ACCOUNT_TAG_NAME "name"
#define PIDGIN_ACCOUNT_TAG_ACCOUNT "account"
#define PIDGIN_ACCOUNT_TAG_PROTOCOL "protocol"
#define PIDGIN_ACCOUNT_TAG_PASSWORD "password"
#define PIDGIN_ACCOUNT_TAG_SETTINGS "settings"
#define PIDGIN_SETTING_PROP_UI "ui"
#define PIDGIN_SETTING_PROP_NAME "name"
#define PIDGIN_SETTING_PROP_TYPE "type"
#define PIDGIN_PROTOCOL_BONJOUR "bonjour"
#define PIDGIN_PROTOCOL_NOVELL "novell"

static void
import_dialog_pidgin_parse_setting (EmpathyImportAccountData *data,
                                    xmlNodePtr setting)
{
  PidginCmMapItem *item = NULL;
  gchar *tag_name;
  gchar *type = NULL;
  gchar *content;
  guint i;
  GValue *value = NULL;

  /* We can't do anything if the setting don't have a name */
  tag_name = (gchar *) xmlGetProp (setting,
      (xmlChar *) PIDGIN_SETTING_PROP_NAME);
  if (!tag_name)
    return;

  /* Search for the map corresponding to setting we are parsing */
  for (i = 0; i < G_N_ELEMENTS (pidgin_cm_map); i++)
    {
      if (!tp_strdiff (data->protocol, pidgin_cm_map[i].protocol) &&
          !tp_strdiff (tag_name, pidgin_cm_map[i].pidgin_name))
        {
          item = pidgin_cm_map + i;
          break;
        }
    }
  g_free (tag_name);

  /* If we didn't find the item, there is nothing we can do */
  if (!item)
    return;

  type = (gchar *) xmlGetProp (setting, (xmlChar *) PIDGIN_SETTING_PROP_TYPE);
  content = (gchar *) xmlNodeGetContent (setting);

  if (!tp_strdiff (type, "bool"))
    {
      i = (gint) g_ascii_strtod (content, NULL);
      value = tp_g_value_slice_new (G_TYPE_BOOLEAN);
      g_value_set_boolean (value, i != 0);
    }
  else if (!tp_strdiff (type, "int"))
    {
      TpConnectionManager *cm = NULL;
      const TpConnectionManagerProtocol *proto;
      const TpConnectionManagerParam *param;
      const gchar *signature;
      int signature_i;

      if (!empathy_import_protocol_is_supported (data->protocol, &cm))
        return;

      proto = tp_connection_manager_get_protocol (cm, data->protocol);
      param = tp_connection_manager_protocol_get_param (proto, item->cm_name);
      signature = tp_connection_manager_param_get_dbus_signature (param);
      signature_i = (int) (*signature);

      i = (gint) g_ascii_strtod (content, NULL);

      if (signature_i == DBUS_TYPE_INT16 ||
          signature_i == DBUS_TYPE_INT32)
        {
          value = tp_g_value_slice_new (G_TYPE_INT);
          g_value_set_int (value, i);
        }
      else if (signature_i == DBUS_TYPE_UINT16 ||
          signature_i == DBUS_TYPE_UINT32)
        {
          value = tp_g_value_slice_new (G_TYPE_UINT);
          g_value_set_uint (value, (guint) i);
        }
    }
  else if (!tp_strdiff (type, "string"))
    {
      value = tp_g_value_slice_new (G_TYPE_STRING);
      g_value_set_string (value, content);
    }

  if (value)
    g_hash_table_insert (data->settings, (gpointer) item->cm_name, value);

  g_free (type);
  g_free (content);
}

static void
import_dialog_pidgin_handle_settings (EmpathyImportAccountData *data,
                                      xmlNodePtr settings)
{
  xmlNodePtr setting;
  gchar *tag_ui, *name, *type, *content;

  tag_ui = (gchar *) xmlGetProp (settings, (xmlChar *) PIDGIN_SETTING_PROP_UI);

  /* UI settings - fetch the Enabled parameter.
   * The expected value of the ui property is 'gtk-gaim', which looks obsolete,
   * but still valid for 2.7.3.
   */
  if (tag_ui && !tp_strdiff (tag_ui, "gtk-gaim"))
    {
      for (setting = settings->children; setting; setting = setting->next)
        {
          name = (gchar *) xmlGetProp (setting,
              (xmlChar *) PIDGIN_SETTING_PROP_NAME);
          type = (gchar *) xmlGetProp (setting,
              (xmlChar *) PIDGIN_SETTING_PROP_TYPE);
          /* The Enabled parameter is supposed to be boolean.
           * Pidgin name of the setting is 'auto-login'.
           */
          if (!tp_strdiff (name, "auto-login") && !tp_strdiff (type, "bool"))
            {
              content = (gchar *) xmlNodeGetContent (setting);
              data->enabled = (0 != (gint) g_ascii_strtod (content, NULL));
              g_free (content);
            }
          g_free (type);
          g_free (name);
        }
    }
  /* General settings. */
  else
    {
      for (setting = settings->children; setting; setting = setting->next)
        import_dialog_pidgin_parse_setting (data, setting);
    }

  g_free (tag_ui);
}

GList *
empathy_import_pidgin_load (void)
{
  xmlNodePtr rootnode, node, child;
  xmlParserCtxtPtr ctxt;
  xmlDocPtr doc;
  gchar *filename;
  GList *accounts = NULL;

  /* Load pidgin accounts xml */
  ctxt = xmlNewParserCtxt ();
  filename = g_build_filename (g_get_home_dir (), ".purple", "accounts.xml",
      NULL);

  if (g_access (filename, R_OK) != 0)
    goto FILENAME;

  doc = xmlCtxtReadFile (ctxt, filename, NULL, 0);

  rootnode = xmlDocGetRootElement (doc);
  if (rootnode == NULL)
    goto OUT;

  for (node = rootnode->children; node; node = node->next)
    {
      EmpathyImportAccountData *data;

      /* If it is not an account node, skip. */
      if (tp_strdiff ((gchar *) node->name, PIDGIN_ACCOUNT_TAG_ACCOUNT))
        continue;

      /* Create account data struct */
      data = empathy_import_account_data_new ("Pidgin");

      /* Parse account's child nodes to fill the account data struct */
      for (child = node->children; child; child = child->next)
        {
          GValue *value;

          /* Protocol */
          if (!tp_strdiff ((gchar *) child->name,
              PIDGIN_ACCOUNT_TAG_PROTOCOL))
            {
              xmlChar *content;
              const gchar *protocol;

              content = xmlNodeGetContent (child);

              protocol = (const gchar *) content;

              if (g_str_has_prefix (protocol, "prpl-"))
                protocol += 5;

              if (!tp_strdiff (protocol, PIDGIN_PROTOCOL_BONJOUR))
                data->protocol = g_strdup ("salut");
              else if (!tp_strdiff (protocol, PIDGIN_PROTOCOL_NOVELL))
                data->protocol = g_strdup ("groupwise");
              else
                data->protocol = g_strdup (protocol);

              xmlFree (content);

              if (data->protocol == NULL)
                break;
            }

          /* Username and IRC server. */
          else if (!tp_strdiff ((gchar *) child->name,
              PIDGIN_ACCOUNT_TAG_NAME))
            {
              gchar *name;
              GStrv name_resource = NULL;
              GStrv nick_server = NULL;
              const gchar *username;

              name = (gchar *) xmlNodeGetContent (child);

              /* Split "username/resource" */
              if (g_strrstr (name, "/") != NULL)
                {
                  name_resource = g_strsplit (name, "/", 2);
                  username = name_resource[0];
                }
              else
                username = name;

             /* Split "username@server" if it is an IRC account */
             if (strstr (name, "@") && !tp_strdiff (data->protocol, "irc"))
              {
                nick_server = g_strsplit (name, "@", 2);
                username = nick_server[0];

                /* Add the server setting */
                value = tp_g_value_slice_new (G_TYPE_STRING);
                g_value_set_string (value, nick_server[1]);
                g_hash_table_insert (data->settings, (gpointer) "server", value);
              }

              /* Add the account setting */
              value = tp_g_value_slice_new (G_TYPE_STRING);
              g_value_set_string (value, username);
              g_hash_table_insert (data->settings, (gpointer) "account", value);

              g_strfreev (name_resource);
              g_strfreev (nick_server);
              g_free (name);
            }

          /* Password */
          else if (!tp_strdiff ((gchar *) child->name,
              PIDGIN_ACCOUNT_TAG_PASSWORD))
            {
              gchar *password;

              password = (gchar *) xmlNodeGetContent (child);

              /* Add the password setting */
              value = tp_g_value_slice_new (G_TYPE_STRING);
              g_value_set_string (value, password);
              g_hash_table_insert (data->settings, (gpointer) "password", value);

              g_free (password);
            }

          /* Other settings */
          else if (!tp_strdiff ((gchar *) child->name,
              PIDGIN_ACCOUNT_TAG_SETTINGS))
            import_dialog_pidgin_handle_settings (data, child);
        }

      /* If we have the needed settings, add the account data to the list,
       * otherwise free the data */
      if (data->protocol != NULL && g_hash_table_size (data->settings) > 0)
        {
          /* Special-case XMPP:
           * http://bugzilla.gnome.org/show_bug.cgi?id=579992 */
          if (!tp_strdiff (data->protocol, "jabber"))
            {
              if (EMP_STR_EMPTY (tp_asv_get_string (data->settings, "server")))
                {
                  g_hash_table_remove (data->settings, "port");
                  g_hash_table_remove (data->settings, "server");
                }
            }

          /* If there is no password then MC treats the account as not
           * ready and doesn't display it. */
          if (!g_hash_table_lookup (data->settings, "password"))
            {
              GValue *value;
              value = tp_g_value_slice_new (G_TYPE_STRING);
              g_value_set_string (value, "");
              g_hash_table_insert (data->settings, (gpointer) "password", value);
            }

          accounts = g_list_prepend (accounts, data);
        }
      else
        empathy_import_account_data_free (data);
    }

OUT:
  xmlFreeDoc (doc);
  xmlFreeParserCtxt (ctxt);

FILENAME:
  g_free (filename);

  return accounts;
}

gboolean
empathy_import_pidgin_accounts_to_import (void)
{
  gchar *filename;
  gboolean out;
  GFile *file;

  filename = g_build_filename (g_get_home_dir (), ".purple", "accounts.xml",
      NULL);
  file = g_file_new_for_path (filename);
  out = g_file_query_exists (file, NULL);

  g_free (filename);
  g_object_unref (file);

  return out;
}
