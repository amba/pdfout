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
  test_init ();
  
  /* test_pdfout (0, 0, "page_labels_test"); */
  
const char *input = "\
[\n\
    {\n\
        \"page\": 1\n\
    }\n\
]\n";

 const char *broken_inputs[] = {"abc", NULL};
  
 test_set_get ("pagelabels", input, input, "[]\n", broken_inputs);
  /* check default filename option */
  {
    char *pdf = test_new_pdf ();
    char *default_filename = pdfout_append_suffix (pdf, ".pagelabels");
    
    test_write_string_to_file (default_filename, input);
    
    test_pdfout (0, 0, "setpagelabels", pdf, "-d");

    xunlink (default_filename);
    
    test_pdfout (0, 0, "getpagelabels", pdf, "-d");

    test_file_matches_string (default_filename, input);
    xunlink (default_filename);
  }
      
  char *pdf = test_new_pdf ();

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
