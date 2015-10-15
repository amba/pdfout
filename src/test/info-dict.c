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
  test_init ();

#define COMMAND "info"
#define INPUT "\
Title: pdfout info dict test\n\
Author: pdfout\n\
Subject: testing\n\
Keywords: la la la\n\
Creator: none\n\
Producer: la la\n\
CreationDate: 20150701\n\
ModDate: D:20150702\n\
Trapped: Unknown\n"

#define BROKEN_INPUT					\
  "Titel: myfile",		/* should be 'Title' */ \
    "Trapped: true",		/* should be 'True' */	\
    "ModDate: D:20151"		/* illegal date */
  
#include "set-get-template.c"
  
  /* non-incremental update */
  {
    char *pdf = test_new_pdf ();
    char *output_pdf = test_tempname ();

    test_pdfout (INPUT, 0, "setinfo", pdf, "-o", output_pdf);

    /* pdf is untouched */
    test_files_equal (pdf, test_data ("empty10.pdf"));

    test_pdfout (0, INPUT, "getinfo", output_pdf);
  }

  /* append option */
  {
    char *pdf = test_new_pdf ();

    test_pdfout (INPUT, 0, "setinfo", pdf);
    test_pdfout ("CreationDate: D:20150804", 0, "setinfo", pdf, "--append");

    test_pdfout (0, "\
Title: pdfout info dict test\n\
Author: pdfout\n\
Subject: testing\n\
Keywords: la la la\n\
Creator: none\n\
Producer: la la\n\
CreationDate: D:20150804\n\
ModDate: D:20150702\n\
Trapped: Unknown\n", "getinfo", pdf);
  }
    
  return 0;
}
