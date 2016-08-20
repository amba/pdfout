#ifndef _GNU_SOURCE
# define _GNU_SOURCE
#endif

#include "shared-with-tests.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <assert.h>

int pdfout_batch_mode = 0;



const char *
pdfout_outline_suffix (enum pdfout_outline_format format)
{
  switch (format)
    {
    case 0: return ".outline.yaml";
    case 1: return ".outline.wysiwyg";
    default:
      abort ();
    }
  error (1, 0, "pdfout_outline_suffix: invalid format");
  return NULL;
}

void
pdfout_vmsg_raw (const char *format, va_list ap)
{
  if (pdfout_batch_mode)
    return;
  vfprintf (stderr, format, ap);
}

void
pdfout_msg_raw (const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  pdfout_vmsg_raw (format, ap);
}

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
pdfout_errno_msg (int errno, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  pdfout_errno_vmsg (errno, format, ap);
}

void
pdfout_errno_vmsg (int errno, const char *format, va_list ap)
{
  if (pdfout_batch_mode)
    return;
  
  vfprintf (stderr, format, ap);

  if (errno)
    fprintf (stderr, ": %s\n", strerror (errno));
  else
    fprintf (stderr, "\n");
}
