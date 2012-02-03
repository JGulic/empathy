/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2004-2007 Imendio AB
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
 *          Richard Hult <richard@imendio.com>
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include <glib/gi18n-lib.h>

#ifdef HAVE_ENCHANT
#include <enchant.h>
#endif

#include "empathy-spell.h"

#define DEBUG_FLAG EMPATHY_DEBUG_OTHER
#include <libempathy/empathy-debug.h>
#include <libempathy/empathy-gsettings.h>

#ifdef HAVE_ENCHANT

typedef struct {
	EnchantBroker *config;
	EnchantDict   *speller;
} SpellLanguage;

#define ISO_CODES_DATADIR    ISO_CODES_PREFIX "/share/xml/iso-codes"
#define ISO_CODES_LOCALESDIR ISO_CODES_PREFIX "/share/locale"

/* Language code (gchar *) -> language name (gchar *) */
static GHashTable  *iso_code_names = NULL;
/* Contains only _enabled_ languages
 * Language code (gchar *) -> language (SpellLanguage *) */
static GHashTable  *languages = NULL;

static void
spell_iso_codes_parse_start_tag (GMarkupParseContext  *ctx,
				 const gchar          *element_name,
				 const gchar         **attr_names,
				 const gchar         **attr_values,
				 gpointer              data,
				 GError              **error)
{
	const gchar *ccode_longB, *ccode_longT, *ccode;
	const gchar *lang_name;

	if (!g_str_equal (element_name, "iso_639_entry") ||
	    attr_names == NULL || attr_values == NULL) {
		return;
	}

	ccode = NULL;
	ccode_longB = NULL;
	ccode_longT = NULL;
	lang_name = NULL;

	while (*attr_names && *attr_values) {
		if (g_str_equal (*attr_names, "iso_639_1_code")) {
			if (**attr_values) {
				ccode = *attr_values;
			}
		}
		else if (g_str_equal (*attr_names, "iso_639_2B_code")) {
			if (**attr_values) {
				ccode_longB = *attr_values;
			}
		}
		else if (g_str_equal (*attr_names, "iso_639_2T_code")) {
			if (**attr_values) {
				ccode_longT = *attr_values;
			}
		}
		else if (g_str_equal (*attr_names, "name")) {
			lang_name = *attr_values;
		}

		attr_names++;
		attr_values++;
	}

	if (!lang_name) {
		return;
	}

	if (ccode) {
		g_hash_table_insert (iso_code_names,
				     g_strdup (ccode),
				     g_strdup (lang_name));
	}

	if (ccode_longB) {
		g_hash_table_insert (iso_code_names,
				     g_strdup (ccode_longB),
				     g_strdup (lang_name));
	}

	if (ccode_longT) {
		g_hash_table_insert (iso_code_names,
				     g_strdup (ccode_longT),
				     g_strdup (lang_name));
	}
}

static void
spell_iso_code_names_init (void)
{
	GError *err = NULL;
	gchar  *buf;
	gsize   buf_len;

	iso_code_names = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_free);

	bindtextdomain ("iso_639", ISO_CODES_LOCALESDIR);
	bind_textdomain_codeset ("iso_639", "UTF-8");

	/* FIXME: We should read this in chunks and pass to the parser. */
	if (g_file_get_contents (ISO_CODES_DATADIR "/iso_639.xml", &buf, &buf_len, &err)) {
		GMarkupParseContext *ctx;
		GMarkupParser        parser = {
			spell_iso_codes_parse_start_tag,
			NULL, NULL, NULL, NULL
		};

		ctx = g_markup_parse_context_new (&parser, 0, NULL, NULL);
		if (!g_markup_parse_context_parse (ctx, buf, buf_len, &err)) {
			g_warning ("Failed to parse '%s': %s",
				   ISO_CODES_DATADIR"/iso_639.xml",
				   err->message);
			g_error_free (err);
		}

		g_markup_parse_context_free (ctx);
		g_free (buf);
	} else {
		g_warning ("Failed to load '%s': %s",
				ISO_CODES_DATADIR"/iso_639.xml", err->message);
		g_error_free (err);
	}
}

static void
spell_notify_languages_cb (GSettings   *gsettings,
			   const gchar *key,
			   gpointer     user_data)
{
	DEBUG ("Resetting languages due to config change");

	/* We just reset the languages list. */
	if (languages != NULL) {
		g_hash_table_unref (languages);
		languages = NULL;
	}
}

static void
empathy_spell_free_language (SpellLanguage *lang)
{
	enchant_broker_free_dict (lang->config, lang->speller);
	enchant_broker_free (lang->config);

	g_slice_free (SpellLanguage, lang);
}

static void
spell_setup_languages (void)
{
	static GSettings *gsettings = NULL;
	gchar  *str;

	if (gsettings == NULL) {
		/* FIXME: this is never uninitialised */
		gsettings = g_settings_new (EMPATHY_PREFS_CHAT_SCHEMA);

		g_signal_connect (gsettings,
			"changed::" EMPATHY_PREFS_CHAT_SPELL_CHECKER_LANGUAGES,
			G_CALLBACK (spell_notify_languages_cb), NULL);
	}

	if (languages) {
		return;
	}

	languages = g_hash_table_new_full (g_str_hash, g_str_equal,
			g_free, (GDestroyNotify) empathy_spell_free_language);

	str = g_settings_get_string (gsettings,
			EMPATHY_PREFS_CHAT_SPELL_CHECKER_LANGUAGES);

	if (str != NULL) {
		gchar **strv;
		gint    i;

		strv = g_strsplit (str, ",", -1);

		i = 0;
		while (strv && strv[i]) {
			SpellLanguage *lang;

			DEBUG ("Setting up language:'%s'", strv[i]);

			lang = g_slice_new0 (SpellLanguage);

			lang->config = enchant_broker_init ();
			lang->speller = enchant_broker_request_dict (lang->config, strv[i]);

			if (lang->speller == NULL) {
				DEBUG ("language '%s' has no valid dict", strv[i]);
			} else {
				g_hash_table_insert (languages,
						     g_strdup (strv[i]),
						     lang);
			}

			i++;
		}

		if (strv) {
			g_strfreev (strv);
		}

		g_free (str);
	}
}

const gchar *
empathy_spell_get_language_name (const gchar *code)
{
	const gchar *name;

	g_return_val_if_fail (code != NULL, NULL);

	if (!iso_code_names) {
		spell_iso_code_names_init ();
	}

	name = g_hash_table_lookup (iso_code_names, code);
	if (!name) {
		return NULL;
	}

	return dgettext ("iso_639", name);
}

static void
enumerate_dicts (const gchar * const lang_tag,
                 const gchar * const provider_name,
                 const gchar * const provider_desc,
                 const gchar * const provider_file,
		 gpointer            user_data)
{
	GList **list = user_data;
	gchar  *lang = g_strdup (lang_tag);

	if (strchr (lang, '_')) {
		/* cut country part out of language */
		strchr (lang, '_')[0] = '\0';
	}

	if (g_list_find_custom (*list, lang, (GCompareFunc) strcmp)) {
		/* this language is already part of the list */
		g_free (lang);
		return;
	}

	*list = g_list_append (*list, g_strdup (lang));
}

GList *
empathy_spell_get_language_codes (void)
{
	EnchantBroker *broker;
	GList         *list_langs = NULL;

	broker = enchant_broker_init ();
	enchant_broker_list_dicts (broker, enumerate_dicts, &list_langs);
	enchant_broker_free (broker);

	return list_langs;
}

GList *
empathy_spell_get_enabled_language_codes (void)
{
	spell_setup_languages ();
	return g_hash_table_get_keys (languages);
}

void
empathy_spell_free_language_codes (GList *codes)
{
	g_list_foreach (codes, (GFunc) g_free, NULL);
	g_list_free (codes);
}

gboolean
empathy_spell_check (const gchar *word)
{
	gint         enchant_result = 1;
	const gchar *p;
	gboolean     digit;
	gunichar     c;
	gint         len;
	GHashTableIter iter;
	SpellLanguage  *lang;

	g_return_val_if_fail (word != NULL, FALSE);

	spell_setup_languages ();

	if (!languages) {
		return TRUE;
	}

	/* Ignore certain cases like numbers, etc. */
	for (p = word, digit = TRUE; *p && digit; p = g_utf8_next_char (p)) {
		c = g_utf8_get_char (p);
		digit = g_unichar_isdigit (c);
	}

	if (digit) {
		/* We don't spell check digits. */
		DEBUG ("Not spell checking word:'%s', it is all digits", word);
		return TRUE;
	}

	len = strlen (word);
	g_hash_table_iter_init (&iter, languages);
	while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &lang)) {
		enchant_result = enchant_dict_check (lang->speller, word, len);

		if (enchant_result == 0) {
			break;
		}
	}

	return (enchant_result == 0);
}

GList *
empathy_spell_get_suggestions (const gchar *code,
			       const gchar *word)
{
	gint   len;
	GList *suggestion_list = NULL;
	SpellLanguage *lang;
	gchar **suggestions;
	gsize   i, number_of_suggestions;

	g_return_val_if_fail (code != NULL, NULL);
	g_return_val_if_fail (word != NULL, NULL);

	spell_setup_languages ();

	if (!languages) {
		return NULL;
	}

	len = strlen (word);

	lang = g_hash_table_lookup (languages, code);
	if (!lang) {
		return NULL;
	}

	suggestions = enchant_dict_suggest (lang->speller, word, len,
					    &number_of_suggestions);

	for (i = 0; i < number_of_suggestions; i++) {
		suggestion_list = g_list_append (suggestion_list,
						 g_strdup (suggestions[i]));
	}

	if (suggestions) {
		enchant_dict_free_string_list (lang->speller, suggestions);
	}

	return suggestion_list;
}

gboolean
empathy_spell_supported (void)
{
	if (g_getenv ("EMPATHY_SPELL_DISABLED")) {
		DEBUG ("EMPATHY_SPELL_DISABLE env variable defined");
		return FALSE;
	}

	return TRUE;
}

void
empathy_spell_add_to_dictionary (const gchar *code,
				 const gchar *word)
{
	SpellLanguage *lang;

	g_return_if_fail (code != NULL);
	g_return_if_fail (word != NULL);

	spell_setup_languages ();
	if (languages == NULL)
		return;

	lang = g_hash_table_lookup (languages, code);
	if (lang == NULL)
		return;

	enchant_dict_add_to_pwl (lang->speller, word, strlen (word));
}

#else /* not HAVE_ENCHANT */

gboolean
empathy_spell_supported (void)
{
	return FALSE;
}

GList *
empathy_spell_get_suggestions (const gchar *code,
                               const gchar *word)
{
	DEBUG ("Support disabled, could not get suggestions");

	return NULL;
}

gboolean
empathy_spell_check (const gchar *word)
{
	DEBUG ("Support disabled, could not check spelling");

	return TRUE;
}

const gchar *
empathy_spell_get_language_name (const gchar *lang)
{
	DEBUG ("Support disabled, could not get language name");

	return NULL;
}

GList *
empathy_spell_get_language_codes (void)
{
	DEBUG ("Support disabled, could not get language codes");

	return NULL;
}

void
empathy_spell_free_language_codes (GList *codes)
{
}

void
empathy_spell_add_to_dictionary (const gchar *code,
				 const gchar *word)
{
	DEBUG ("Support disabled, could not expand the dictionary");
}

GList *
empathy_spell_get_enabled_language_codes (void)
{
	DEBUG ("Support disabled, could not get enabled language codes");

	return NULL;
}

#endif /* HAVE_ENCHANT */


void
empathy_spell_free_suggestions (GList *suggestions)
{
	g_list_foreach (suggestions, (GFunc) g_free, NULL);
	g_list_free (suggestions);
}

