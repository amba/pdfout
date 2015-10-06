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
  {
    char *pdf = test_cp ("broken.pdf", NULL);
  
    test_pdfout_status (1, 0, 0, "repair", "--check", pdf);
  
    test_pdfout (0, 0, "repair", pdf);

    test_pdfout (0, 0, "repair", "-c", pdf);
  }

  /* --output option */
  {
    char *pdf = test_cp ("broken.pdf", NULL);
    char *output = test_tempname ();
    test_pdfout (0, 0, "repair", pdf, "-o", output);
    test_pdfout (0, 0, "repair", "-c", output);

    /* pdf is untouched  */
    test_files_equal (pdf, test_data ("broken.pdf"));
  }
  return 0;
}
