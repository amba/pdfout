#ifndef SHARED_WITH_TESTS_H
#define SHARED_WITH_TESTS_H

#include <stdbool.h>
#include <stdarg.h>

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define PDFOUT_PRINTFLIKE(index)			\
  __attribute__ ((format (printf, index, index + 1)))
# define PDFOUT_WUR __attribute__ ((warn_unused_result))
# define PDFOUT_UNUSED __attribute__ ((__unused__))
# define PDFOUT_NORETURN __attribute__ ((__noreturn__))
#else
# define PDFOUT_PRINTFLIKE(index) /* empty */
# define PDFOUT_WUR
# define PDFOUT_UNUSED
# define PDFOUT_NORETURN
#endif

/* if set, do not print any status messages to stderr  */
extern int pdfout_batch_mode;

enum pdfout_outline_format {
  PDFOUT_OUTLINE_YAML,
  PDFOUT_OUTLINE_WYSIWYG
};

/* returns statically allocated string */
const char *pdfout_outline_suffix (enum pdfout_outline_format);


void pdfout_vmsg_raw (const char *format, va_list ap);
  
void pdfout_msg_raw (const char *format, ...)
  PDFOUT_PRINTFLIKE (1);
  
void pdfout_msg (const char *fmt, ...)
  PDFOUT_PRINTFLIKE (1);

void pdfout_vmsg (const char *format, va_list ap);

void pdfout_errno_msg (int errnum, const char *format, ...)
 PDFOUT_PRINTFLIKE (2);

void pdfout_errno_vmsg (int errnum, const char *format, va_list ap);
  
#endif	/* !SHARED_WITH_TESTS_H */
