#ifndef SHARED_WITH_TESTS_H
#define SHARED_WITH_TESTS_H

#include <stdbool.h>
#include <stdarg.h>

/* if set, do not print any status messages to stderr  */
extern int pdfout_batch_mode;

/* append SUFFIX to FILENAME. */
char *pdfout_append_suffix (const char *filename, const char *suffix);

enum pdfout_outline_format {
  PDFOUT_OUTLINE_YAML,
  PDFOUT_OUTLINE_WYSIWYG
};

/* returns statically allocated string */
const char *pdfout_outline_suffix (enum pdfout_outline_format)
  _GL_ATTRIBUTE_PURE;

/* Shall be used for all status messages.  */
#define pdfout_msg(format, args...)		\
  do						\
    {						\
      if (pdfout_batch_mode == 0)		\
	error (0, 0, format, ## args);		\
    }						\
  while (0)

#define pdfout_errno_msg(errno, format, args...)	\
  if (pdfout_batch_mode == 0) \
    error (0, errno, format, ## args);

#endif	/* !SHARED_WITH_TESTS_H */
