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
#include "string.h"
int
main (void)
{
  test_init ();

  /* pagecount */
  {
    char *file = test_tempname ();
    test_pdfout (0, 0, "create", "-p3", "-o", file);
    test_fgrep (file, "Count 3");
  }
		   
  /* grep for width and height in page dict */
  {
    const char *paper_sizes[][2] = {
      /* {format, "width"} */
      {"", "0 0 595.* 841"},		/* A4 */
      {"A0", "0 0 2383.* 3370"},
      {"B1", "0 0 2004.* 2834"},
      {"C2", "0 0 1298.* 1836"},
      {"2A0", "0 0 3370.* 4767"},
      {"ANSI A", "0 0 612 792"},
      {0}
    };

    const char *(*ptr)[2];

    for (ptr = paper_sizes; **ptr; ++ptr)
      {
	const char *size = ptr[0][0];
	const char *grep_string = ptr[0][1];
	char *file = test_tempname ();
	if (size[0] == '\0')
	  /* use default paper size */
	  test_pdfout (0, 0, "create", "-o", file);
	else
	  test_pdfout (0, 0, "create", "-o", file, "-s", size);

	test_grep (file, grep_string);
      }
  }
  /* landscape option */
  {
    char *file = test_tempname ();
    test_pdfout (0, 0, "create", "-sANSI A", "-l", "-o", file);
    test_fgrep (file, "0 0 792 612");
  }
  /* height and width options */
  {
    char *file = test_tempname ();
    test_pdfout (0, 0, "create", "-w", "100", "-h", "100.1", "-o", file);
    test_fgrep (file, "0 0 100 100.1");
  }

  /* invalid usage */
  {
    /* unknown paper sizes */
    const char *paper_sizes[] = {
      "A11",
      "Fettie",
      "a-1",
      NULL
    };

    const char **ptr;
    
    for (ptr = paper_sizes; *ptr; ++ptr)
      test_pdfout_status (EX_USAGE, 0, 0, "create", "-s", *ptr);

    /* no height */
    test_pdfout_status (EX_USAGE, 0, 0, "create", "-w", "100");

    /* negative height */
    test_pdfout_status (EX_USAGE, 0, 0, "create", "w", "100", "-h", "-100");
    
  }
  return 0;
}
