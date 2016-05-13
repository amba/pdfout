/* 

   Copyright (C) 1990-2000, 2003-2004, 2006-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


/* Derived from GNU gnulib's xalloc module.  */

#include "common.h"

void *
pdfout_x2nrealloc_imp (fz_context *ctx, void *p, int *pn, unsigned s)
{
  int n = *pn;

  if (! p)
    {
      if (! n)
        {
          /* The approximate size to use for initial small allocation
             requests, when the invoking code specifies an old size of
             zero.  This is the largest "small" request for the GNU C
             library malloc.  */
          enum { DEFAULT_MXFAST = 64 * sizeof (size_t) / 4 };

          n = DEFAULT_MXFAST / s;
          n += !n;
        }
    }
  else
    {
      /* Set N = floor (1.5 * N) + 1 so that progress is made even if N == 0.
         Check for overflow, so that N * S stays in int range.
         The check may be slightly conservative, but an exact check isn't
         worth the trouble.  */
      if (INT_MAX / 3 * 2 / s <= n)
        pdfout_throw (ctx, "int overflow in x2nrealloc");
      n += n / 2 + 1;
    }

  *pn = n;
  return fz_resize_array (ctx, p, n, s);
}
