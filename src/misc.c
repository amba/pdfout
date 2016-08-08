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


#include "common.h"
#include <string.h>
#include <xalloc.h>
#include <exitfail.h>
#include <c-ctype.h>
#include <argmatch.h>
#include <unistr.h>


int
pdfout_strtoui (fz_context *ctx, const char *s)
{
  char *tailptr;
  long rv = strtol (s, &tailptr, 10);
  if (rv < 0 || tailptr[0] != 0 || tailptr == s)
    pdfout_throw (ctx, "not a positive int: '%s'", s);
  if (rv > INT_MAX)
    pdfout_throw (ctx, "integer overflow: '%s'", s);
  return rv;
}

int
pdfout_strtoint (fz_context *ctx, const char *nptr, char **endptr)
{
  long rv = strtol (nptr, endptr, 10);
  if (rv > INT_MAX || rv < INT_MIN)
    pdfout_throw (ctx, "int overflow with argument '%s'", nptr);
  return rv;
}

int
pdfout_strtoint_old (const char *nptr, char **endptr)
{
  long rv = strtol (nptr, endptr, 10);
  if (rv > INT_MAX || rv < INT_MIN)
    {
      if (endptr)
	*endptr = (char *) nptr;
      return rv > 0 ? INT_MAX : INT_MIN;
    }
  return rv;
}

int
pdfout_strtoint_null (fz_context *ctx, const char *string)
{
  int result;
  char *endptr;
  result = pdfout_strtoint (ctx, string, &endptr);
  if (endptr == string || endptr[0] != '\0')
    pdfout_throw (ctx, "pdfout_strtoint_null: invalid int: '%s'", string);
  return result;
}

int
pdfout_strtoint_null_old( const char *string)
{
  int result;
  char *endptr;
  result = pdfout_strtoint_old (string, &endptr);
  if (endptr == string || endptr[0] != '\0')
    error (1, 0, "pdfout_strtoint_null: invalid int: '%s'", string);
  return result;
}

float
pdfout_strtof (fz_context *ctx, const char *string)
{
  float result;
  char *endptr;

  errno = 0;

  result = strtof (string, &endptr);

  if (errno || endptr == string || endptr[0] != '\0')
    pdfout_throw_errno (ctx, "pdfout_strtof: invalid float: '%s'", string);
  
  return result;
}

float
pdfout_strtof_old (const char *string)
{
  float result;
  char *endptr;

  errno = 0;

  result = strtof (string, &endptr);

  if (errno || endptr == string || endptr[0] != '\0')
    error (1, errno, "pdfout_strtof: invalid float: '%s'", string);
  
  return result;
}

float
pdfout_strtof_nan (const char *string)
{
  float result;
  char *endptr;
  errno = 0;
  result = strtof (string, &endptr);
  if (errno || endptr == string || endptr[0] != '\0')
    {
      error (1, 0,  "pdfout_strtof_nan: invalid float: '%s'", string);
      result = nanf ("");
    }
  return result;
}

void
pdfout_vthrow (fz_context *ctx, const char *fmt, va_list ap)
{
  /* fz_throw does not use a C99 printf internally, so format here.  */
  char buffer[4096];
  pdfout_vsnprintf (ctx, buffer, fmt, ap);
  fz_throw (ctx, FZ_ERROR_GENERIC, "%s", buffer);
}

void
pdfout_throw (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  pdfout_vthrow (ctx, fmt, ap);
}

void
pdfout_warn (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  /* fz_warn does not use a C99 printf internally, so format here.  */
  char buffer[4096];
  pdfout_vsnprintf (ctx, buffer, fmt, ap);
  fz_warn (ctx, "%s", buffer);
}
  
void
pdfout_throw_errno (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  
  if (errno)
    {
      char buf[2048];
      pdfout_snprintf (ctx, buf, "%s: %s", fmt, strerror (errno));
      pdfout_vthrow (ctx, buf, ap);
    }
  
  pdfout_vthrow (ctx, fmt, ap);
}

int
pdfout_snprintf_imp (fz_context *ctx, char *buf, int size,
		     const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  return pdfout_vsnprintf_imp (ctx, buf, size, fmt, ap);
}

int
pdfout_vsnprintf_imp (fz_context *ctx, char *buf, int size,
		      const char *fmt, va_list ap)
{
  errno = 0;
  int ret = vsnprintf (buf, size, fmt, ap);
  if (ret < 0)
    pdfout_throw_errno (ctx, "output error in pdfout_snprintf");
  if (ret >= size)
    pdfout_throw (ctx, "string truncated in pdfout_snprintf");
  return ret;
}


int
pdfout_snprintf_old (char *str, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;
  va_start (ap, fmt);
  errno = 0;
  ret = vsnprintf (str, size, fmt, ap);
  if (ret < 0)
    {
      pdfout_errno_msg (errno, "pdfout_snprintf_old: output error");
      goto error;
    }
  if ((size_t) ret >= size)
    {
      pdfout_msg ("pdfout_snprintf_old: string truncated");
      goto error;
    }
  
  return ret;
 error:
  exit (exit_failure);
}

char *
pdfout_check_utf8 (const char *s, size_t n)
{
  return (char *) u8_check ((const uint8_t *) s, n);
}
