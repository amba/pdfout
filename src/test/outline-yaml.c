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
#include "data/outline-yaml.h"

int
main (int arg, char **argv)
{
  test_init ();

#define COMMAND "outline"
#define INPUT OUTLINE_YAML_10_PAGES

#define BROKEN_INPUT							\
  "[{page: 1}]", /* no title */						\
    "[{title: title, page: 11}]", /* page number excceedes page count */ \
    "[{title: abc, page: 2147483648}]", /* INT_MAX overflow */		\
    "[{title: abc, page: 1, open: lala}]" /* open not a bool */
    
  
#include "set-get-template.c"

  
  /* default destination is [XYZ, null, null, null] */
#define OUTLINE "[{title: 1,  page: 1}]"
  {

    char *pdf = test_new_pdf ();
    test_pdfout (OUTLINE, 0, "setoutline", pdf);
    test_pdfout (0, "\
-   title: 1\n\
    page: 1\n\
    view: [XYZ, null, null, null]\n", "getoutline", pdf);
  }

  /* set default view */
  {
    char *pdf = test_new_pdf ();
    test_pdfout (OUTLINE, 0, "setoutline", pdf,
		 "--default-view=[FitR, 1, 2, 3, 4]");
    test_pdfout (0, "\
-   title: 1\n\
    page: 1\n\
    view: [FitR, 1, 2, 3, 4]\n", "getoutline", pdf);
  }

  /* exit status for invalid default view */
  {
    char *pdf = test_new_pdf ();
    test_pdfout_status (EX_DATAERR, OUTLINE, 0, "setoutline", pdf,
			"--default-view", "[Fit, null]");
  }
  return 0;
}


