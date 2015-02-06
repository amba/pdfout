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
   COMMAND, e.g. "outline" or "outline", "-fw"
   INPUT, e.g. outline10.yaml
   EXPECTED e.g. "outline10.wysiwyg.expected" (optional)
   BROKEN_INPUT comma-separated list of broken input files
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

/* compare after set and get */
{
  char *pdf = test_new_pdf ();
  
  test_pdfout (INPUT, NULL, "set" COMMAND, pdf);
  test_pdfout (NULL, EXPECTED, "get" COMMAND, pdf);
}

/* remove */
{
  char *pdf = test_new_pdf ();

  test_pdfout (INPUT, 0, "set" COMMAND, pdf);
  test_pdfout (0, 0, "set" COMMAND, pdf, "--remove");
  test_pdfout_status (1, 0, 0, "get" COMMAND, pdf);
}

/* return value for getCOMMAND is 1 for empty pdf */
{
  char *pdf = test_new_pdf ();
  test_pdfout_status (1, 0, 0, "get" COMMAND, pdf);
}

/* exit status is EX_DATAERR for all broken input files */
{
  char *broken_input_files[] = {
    BROKEN_INPUT,
    NULL
  };
    
  char *pdf = test_new_pdf ();
  char **input_ptr;
  for (input_ptr = broken_input_files; *input_ptr; ++input_ptr)
    {
      test_pdfout_status (EX_DATAERR, *input_ptr, 0, "set" COMMAND, pdf);
      
      /* pdf is untouched */
      test_files_equal (pdf, test_data ("empty10.pdf"));
    }
}

