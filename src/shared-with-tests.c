/* The pdfout document modification and analysis tool.
   Copyright (C) 2015 AUTHORS (see AUTHORS file)
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include <config.h>

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
