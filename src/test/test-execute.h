#ifndef HAVE_PDFOUT_TEST_EXECUTE_H
#define HAVE_PDFOUT_TEST_EXECUTE_H

extern char *test_prog;

#ifndef TEST_DATA_DIR
# error TEST_DATA_DIR is not defined
#endif

#ifndef TEST_PROG_NAME
# error TEST_PROG_NAME is not defined
#endif

#define TEST_STRINGIFY_POST(arg) #arg
#define TEST_STRINGIFY(arg) TEST_STRINGIFY_POST (arg)

#define TEST_PROG TEST_STRINGIFY (TEST_PROG_NAME)

/* use of valgrind is enabled at run time in 'test_init' function */
extern bool use_valgrind;

/* this declaration is needed on FreeBSD. FIXME: use 'environ' gnulib module?*/
extern char **environ;


#define TEST_VALGRIND \
  "valgrind", "-q", "--error-exitcode=126", "--leak-check=full",	\
    "--suppressions=" TEST_STRINGIFY(TEST_DATA_DIR) "/valgrind.supp"

enum test_search_mode {
  TEST_EXACT,
  TEST_FIXED,
  TEST_GREP,
  TEST_EGREP,
};

/* run COMMAND in a pipe. write string INPUT to the child's stdin and compare
   the child's stdout with string EXPECTED.
   compare child's exit status with EXPECTED_STATUS. */
void _test_command (const char *source_file, int source_line,
		    int expected_status, const char *input,
		    const char *expected, enum test_search_mode mode,
		    const char *command, ...);
#define _test_pdfout_status(status, input, expected, search_mode, args...) \
  do									\
    {									\
      if (use_valgrind)							\
	_test_command (__FILE__, __LINE__, status, input, expected,	\
		       search_mode, TEST_VALGRIND, TEST_PROG, ## args,	\
		       (char *) NULL);					\
      else								\
	_test_command (__FILE__, __LINE__, status, input, expected,	\
		       search_mode, TEST_PROG, ## args, (char *) NULL); \
    } while (0)

#define _test_pdfout(input, expected, search_mode, args...)\
  _test_pdfout_status(0, input, expected, search_mode, ## args)

#define test_pdfout_status(status, input, expected, args...)	\
  _test_pdfout_status (status, input, expected, TEST_EXACT, ## args)

#define test_pdfout(input, expected, args...)		\
  _test_pdfout (input, expected, TEST_EXACT, ## args)

/* search for patterns in pdfout's output */

#define test_pdfout_grep(input, regex, args...)		\
  _test_pdfout (input, regex, TEST_GREP, ## args)

#define test_pdfout_egrep(input, regex, args...) \
  _test_pdfout (input, regex, TEST_EGREP, ## args)

#define test_pdfout_fgrep(input, string, args...)		\
  _test_pdfout (input, string, TEST_FIXED, ## args)

#endif /* ! HAVE_PDFOUT_TEST_EXECUTE_H */
