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

int
main (void)
{
  test_init ("create");
  char *pdf = test_tempname ();

  /* pagecount */
  {
    test_pdfout (0, 0, "create", "-p10", "-o", pdf);
    test_pdfout (0, "10\n", "pagecount", pdf);
  }
		   
  /* grep for width and height in page dict */
  {
    char *paper_sizes[][2] = {
      /* {format, "width"} */
      {"", "0 0 595.* 841"},		/* A4 */
      {"A0", "0 0 2383.* 3370"},
      {"B1", "0 0 2004.* 2834"},
      {"C2", "0 0 1298.* 1836"},
      {"2A0", "0 0 3370.* 4767"},
      {"ANSI A", "0 0 612 792"},
      {0}
    };

    char *(*ptr)[2];

    for (ptr = paper_sizes; **ptr; ++ptr)
      {
	char *size = ptr[0][0];
	char *grep_string = ptr[0][1];

	if (size[0] == '\0')
	  /* use default paper size */
	  test_pdfout_grep (0, grep_string, "create");
	else
	  test_pdfout_grep (0, grep_string, "create", "-s", size);
	
      }
  }
  /* landscape option */
  {
    test_pdfout_fgrep (0, "0 0 792 612", "create", "-sANSI A", "-l");
  }
  /* height and width options */
  {
    test_pdfout_fgrep (0, "0 0 100 100.1", "create", "-w", "100", "-h",
		       "100.1");
  }

  /* invalid usage */
  {
    /* unknown paper sizes */
    char *paper_sizes[] = {
      "A11",
      "Fettie",
      "a-1",
      NULL
    };

    char **ptr;
    
    for (ptr = paper_sizes; *ptr; ++ptr)
      test_pdfout_status (EX_USAGE, 0, 0, "create", "-s", *ptr, "-o", pdf);

    /* no height */
    test_pdfout_status (EX_USAGE, 0, 0, "create", "-w", "100", "-o", pdf);

    /* negative height */
    test_pdfout_status (EX_USAGE, 0, 0, "create", "w", "100", "-h", "-100",
			"-o", pdf);
    
  }
  return 0;
}
