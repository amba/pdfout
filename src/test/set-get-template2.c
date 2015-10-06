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


/* before inclusion, define:
   COMMAND
   INPUT
   EXPECTED
   BROKEN_INPUT
   BROKEN_FILE
   BROKEN_EXPECTED
   REALLY_BROKEN_FILE
*/

#ifndef COMMAND
# error COMMAND undefined
#endif

#ifndef INPUT
# error INPUT undefined
#endif

#ifndef EXPECTED
# define EXPECTED INPUT
#endif

#ifndef BROKEN_INPUT
# error BROKEN_INPUT undefined
#endif

#ifndef BROKEN_FILE
# error BROKEN_FILE undefined
#endif

#ifndef BROKEN_EXPECTED
# error BROKEN_EXPECTED undefined
#endif

#ifndef REALLY_BROKEN_FILE
# error REALLY_BROKEN_FILE undefined
#endif

/* Compare after set and get.  */
{
  char *pdf = test_new_pdf ();
  
  test_pdfout (INPUT, 0, "set" COMMAND, pdf);
  test_pdfout (0, EXPECTED, "get" COMMAND, pdf);
}

/* Remove.  */
{
  char *pdf = test_new_pdf ();

  test_pdfout (INPUT, 0, "set" COMMAND, pdf);
  test_pdfout (0, 0, "set" COMMAND, pdf, "--remove");
  test_pdfout_status (1, 0, 0, "get" COMMAND, pdf);
}

/* Return value for getCOMMAND is 1 for empty pdf.  */
{
  char *pdf = test_new_pdf ();
  test_pdfout_status (1, 0, 0, "get" COMMAND, pdf);
}

/* Exit status is EX_DATAERR for BROKEN_INPUT.  */
{
    
  char *pdf = test_new_pdf ();
  test_pdfout_status (EX_DATAERR, BROKEN_INPUT, 0, "set" COMMAND, pdf);
  
  /* pdf is untouched.  */
  test_files_equal (pdf, test_data ("empty10.pdf"));
}

/* Return value is 2 for BROKEN_FILE.  */
{
  char *pdf = test_data (BROKEN_FILE);
  test_pdfout_status (2, 0, BROKEN_EXPECTED, "get" COMMAND, pdf);
}

/* Return value is 3 for REALLY_BROKEN_FILE.  */
{
  char *pdf = test_data (REALLY_BROKEN_FILE);
  test_pdfout_status (3, 0, 0, "get" COMMAND, pdf);
}
