#include "test.h"

void
_test_streq (const char *file, int line, const char *string1,
	     const char *string2)
{
  if (strcmp (string1, string2) == 0)
    return;
  fprintf (stderr, "%s:%d: strings not equal: got: '%s', expected: '%s'\n",
	   file, line, string1, string2);
  exit (1);
}
