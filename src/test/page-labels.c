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
#include <unistd.h>
#include <errno.h>

static void xunlink (const char *file)
{
  if (unlink (file))
    error (99, errno, "unlink");
}

int
main (void)
{
  test_init ("page-labels");

#define COMMAND "pagelabels"
#define SUFFIX ".pagelabels"
#define INPUT "\
-   page: 1\n\
    prefix: Cover\n\
-   page: 2\n\
-   page: 3\n\
    style: roman\n\
-   page: 4\n\
    style: Roman\n\
-   page: 5\n\
    style: arabic\n\
-   page: 6\n\
    style: letters\n\
-   page: 7\n\
    style: Letters\n\
-   page: 8\n\
    style: arabic\n\
    first: 2\n\
-   page: 9\n"
  
#define BROKEN_INPUT 						\
  "[{page: 2147483648}]",     /* page number overflow */		\
  "[{prefix: FAIL}]"       /* missing page number */
  
#include "set-get-template.c"
  
  /* check default filename option */
  {
    char *pdf = test_new_pdf ();
    char *default_filename = pdfout_append_suffix (pdf, ".pagelabels");
    char *string = "-   page: 1\n";

    test_write_string_to_file (default_filename, string);
    
    test_pdfout (0, 0, "setpagelabels", pdf, "-d");

    xunlink (default_filename);
    
    test_pdfout (0, 0, "getpagelabels", pdf, "-d");

    test_file_matches_string (default_filename, string);
    xunlink (default_filename);
  }
      
  /* check text string options */
  char *pdf = test_new_pdf ();

  {
    test_pdfout ("[{page: 1, prefix: Guten Morgen}]", 0, "setpagelabels", pdf);
    test_fgrep (pdf, "Guten Morgen");

    test_pdfout_status (1, "[{page: 1, prefix: αβγ}]", 0, "setpagelabels", pdf,
			"--pdfdoc-text-strings");

    test_pdfout_status (1, "[{page: 1, prefix: αβγ}]", 0, "setpagelabels", pdf,
			"--pdfdoc-text-strings=ERROR");
	
    test_pdfout ("[{page: 1, prefix: abcαβγdef}]", 0, "setpagelabels", pdf,
		 "--pdfdoc-text-strings=Q");

    test_fgrep (pdf, "abc???def");

    /* invalid arg */
    test_pdfout_status (EX_USAGE, "[{page: 1, prefix: αβγ}]", 0,
			"setpagelabels", pdf, "--pdfdoc-text-strings=ERRORS");
  }

  /* invalid input file */
  {
    test_pdfout_status (EX_USAGE, 0, 0,
			"setpagelabels", pdf, "ugcnugcpmgulcno.yaml");
  }
  
  /* invalid pdf */
  {
    test_pdfout_status (EX_USAGE, "[{page: 1}]", 0,
			"setpagelabels", "uiveLEVU.pdf");
  }
  return 0;
}
