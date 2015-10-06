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

int
pdfout_strtoui (const char *s)
{
  char *tailptr;
  long rv = strtol (s, &tailptr, 10);
  if (rv < 0 || tailptr[0] != 0 || tailptr == s)
    {
      pdfout_msg ("not a positive int: '%s'", s);
      return -1;
    }
  if (rv > INT_MAX)
    {
      pdfout_msg ("integer overflow: '%s'", s);
      return -1;
    }
  return rv;
}

int
pdfout_strtoint (const char *nptr, char **endptr)
{
  long rv = strtol (nptr, endptr, 10);
  if (rv > INT_MAX || rv < INT_MIN)
    {
      pdfout_msg ("pdfout_strtoint: int overflow with argument '%s'", nptr);
      if (endptr)
	*endptr = (char *) nptr;
      return rv > 0 ? INT_MAX : INT_MIN;
    }
  return rv;
}

int
pdfout_strtoint_null (const char *string)
{
  int result;
  char *endptr;
  result = pdfout_strtoint (string, &endptr);
  if (endptr == string || endptr[0] != '\0')
    {
      pdfout_msg ("pdfout_strtoint_null: invalid int: '%s'", string);
      exit (1);
    }
  return result;
}

float
pdfout_strtof (const char *string)
{
  float result;
  char *endptr;

  errno = 0;

  result = strtof (string, &endptr);

  if (errno || endptr == string || endptr[0] != '\0')
    {
      pdfout_errno_msg (errno, "pdfout_strtof: invalid float: '%s'", string);
      exit (1);
    }
  
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
      pdfout_errno_msg (errno, "pdfout_strtof_nan: invalid float: '%s'",
			string);
      result = nanf ("");
    }
  return result;
}

int
pdfout_snprintf (char *str, size_t size, const char *fmt, ...)
{
  va_list ap;
  int ret;
  va_start (ap, fmt);
  errno = 0;
  ret = vsnprintf (str, size, fmt, ap);
  if (ret < 0)
    {
      pdfout_errno_msg (errno, "pdfout_snprintf: output error");
      goto error;
    }
  if ((size_t) ret >= size)
    {
      pdfout_msg ("pdfout_snprintf: string truncated");
      goto error;
    }
  
  return ret;
 error:
  exit (exit_failure);
}

  
