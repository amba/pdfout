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
#include <xalloc.h>
#include <progname.h>

int pdfout_batch_mode = 0;

char *
pdfout_append_suffix (const char *filename, const char *suffix)
{
  size_t filename_len = strlen (filename);
  size_t suffix_len = strlen (suffix);
  char *result = xcharalloc (filename_len + suffix_len + 1);
  
  memcpy (result, filename, filename_len);
  memcpy (result + filename_len, suffix, suffix_len + 1);
  return result;
}

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
