#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "test-helper.h"

#define DEBUG_FLAG EMPATHY_DEBUG_TESTS
#include <libempathy/empathy-debug.h>

#include <libempathy-gtk/empathy-live-search.h>

typedef struct
{
  const gchar *string;
  const gchar *prefix;
  gboolean should_match;
} LiveSearchTest;

static void
test_live_search (void)
{
  LiveSearchTest tests[] =
    {
      /* Test word separators and case */
      { "Hello World", "he", TRUE },
      { "Hello World", "wo", TRUE },
      { "Hello World", "lo", FALSE },
      { "Hello World", "ld", FALSE },
      { "Hello-World", "wo", TRUE },
      { "HelloWorld", "wo", FALSE },

      /* Test composed chars (accentued letters) */
      { "Jörgen", "jor", TRUE },
      { "Gaëtan", "gaetan", TRUE },
      { "élève", "ele", TRUE },
      { "Azais", "AzaÏs", TRUE },

      /* Test decomposed chars, they looks the same, but are actually
       * composed of multiple unicodes */
      { "Jorgen", "Jör", TRUE },
      { "Jörgen", "jor", TRUE },

      /* Multi words */
      { "Xavier Claessens", "Xav Cla", TRUE },
      { "Xavier Claessens", "Cla Xav", TRUE },
      { "Foo Bar Baz", "   b  ", TRUE },
      { "Foo Bar Baz", "bar bazz", FALSE },

      { NULL, NULL, FALSE }
    };
  guint i;

  DEBUG ("Started");
  for (i = 0; tests[i].string != NULL; i ++)
    {
      gboolean match;
      gboolean ok;

      match = empathy_live_search_match_string (tests[i].string, tests[i].prefix);
      ok = (match == tests[i].should_match);

      DEBUG ("'%s' - '%s' %s: %s", tests[i].string, tests[i].prefix,
          tests[i].should_match ? "should match" : "should NOT match",
          ok ? "OK" : "FAILED");

      g_assert (ok);
    }
}

int
main (int argc,
    char **argv)
{
  int result;

  test_init (argc, argv);

  g_test_add_func ("/live-search", test_live_search);

  result = g_test_run ();
  test_deinit ();

  return result;
}
