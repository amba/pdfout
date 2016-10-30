#include "shared-with-tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <assert.h>

void
pdfout_msg (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  pdfout_vmsg (format, ap);
}

void pdfout_vmsg (const char *format, va_list ap)
{
  pdfout_errno_vmsg (0, format, ap);
}

void
pdfout_errno_vmsg (int errno, const char *format, va_list ap)
{
  vfprintf (stderr, format, ap);

  if (errno)
    fprintf (stderr, ": %s\n", strerror (errno));
  else
    fprintf (stderr, "\n");
}
