#ifndef HAVE_PDFOUT_PROGRAM_TEST_H
#define HAVE_PDFOUT_PROGRAM_TEST_H
#include "shared.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define test_assert(expr)					\
  do								\
    {								\
      if (!(expr))						\
	{							\
	  fprintf (stderr, "%s:%d: assertion '%s' failed\n",	\
		   __FILE__, __LINE__, #expr);			\
	  exit (1);					       	\
	}							\
    }								\
  while (0)

#define test_ok(call) test_assert ((call) == 0)
#define test_is(value, call) test_assert ((call) == (value))

void _test_streq (const char *file, int line, const char *string1,
		  const char *string2);

#define test_streq(string1, string2) \
  _test_streq (__FILE__, __LINE__, string1, string2)

#define test_memeq(string1, string2, n) test_ok (memcmp (string1, string2, n))

#endif
