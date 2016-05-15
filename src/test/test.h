#ifndef HAVE_PDFOUT_TEST_H
#define HAVE_PDFOUT_TEST_H

#include <config.h>

#include "../shared-with-tests.h"

#include <sysexits.h>
#include <error.h>
#include <stddef.h>

#include "test-execute.h"

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

#define test_streq(string1, string2) (strcmp (string1, string2) == 0)

#define test_memeq(string1, string2, n) \
  (memcmp (string1, string2, n) == 0)

#define test_init() _test_init (__FILE__)
void _test_init (const char *file_name);

/* generate and register a filename in the tests
   temporary directory.
*/
char *test_tempname (void);

/* copy ${srcdir}/empty10.pdf into the tests temporary directory */
char *test_new_pdf (void);

/* copy file from $srcdir to TMP_FILE with cp(1). TMP_FILE must be in the
   test's temporary directory.
   Calls test_register_temp on TMP_FILE.
   If TMP_FILE is NULL, creates a new random filename in the test's temporary
   directory.  */
char *test_cp (const char *file, char *tmp_file);

/* create FILE with contents STRING */
void test_write_string_to_file (const char *file, const char *string);

/* concatenate $srcdir with FILE   */
char *test_data (const char *file);

void test_set_get (const char *command, const char *input, const char *expected,
		   const char *empty, const char **broken_inputs);

/* call exit (1) if FILE1 and FILE2 differ.
   If they differ, run diff on them to show the differences*/
void test_files_equal (const char *file1, const char *file2);


/* search for patterns in files */
void _test_grep (const char *file, const char *pattern,
		 enum test_search_mode mode);

#define test_grep(file, pattern) \
  _test_grep (file, pattern, TEST_GREP)

#define test_egrep(file, pattern) \
  _test_grep (file, pattern, TEST_EGREP)

#define test_fgrep(file, pattern) \
  _test_grep (file, pattern, TEST_FIXED)

#define test_file_matches_string(file, string) \
  _test_grep (file, string, TEST_EXACT)

#endif	/* !HAVE_PDFOUT_TEST_H */
