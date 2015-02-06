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


#include "test.h"
#include "data/gettxt.h"

int
main (void)
{
  test_init ("gettxt-plain");

  char *pdf = test_data ("hello-world.pdf");
      
  {
    test_pdfout (0, HELLO_WORLD_TXT, "gettxt", pdf);
  }

  /* page ranges */
  
  {
    test_pdfout (0, HELLO_WORLD_TXT_PAGE_1, "gettxt", pdf, "-p1");
    test_pdfout (0, HELLO_WORLD_TXT, "gettxt", pdf, "-p1, 2-3");
  }

  /* invalid page ranges */
  char *invalid_ranges[] = {
    "",      /* empty token */
    ",1",    /* empty token */
    "1,",    /* empty token */
    "1 ",    /* trailing space */
    "1,a",   /* not a base 10 integer */
    "2-1",   /* wrong order */
    "11",    /* page count is 10 */
    "11-12",    /* page count is 10 */
    "1-11",  /* page count is 10 */
    "-1-2",  /* negative ? */
    "1-2a",  /* not an integer */
    NULL
  };

  {
    char **invalid;
    for (invalid = invalid_ranges; *invalid; ++invalid)
      test_pdfout_status (EX_USAGE, 0, 0, "gettxt", pdf, "-p", *invalid);
  }
  return 0;
}
