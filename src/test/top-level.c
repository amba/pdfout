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
main (int argc, char **argv)
{
  test_init ();

  test_pdfout_fgrep (0, "\n  -V, --version", "-h");
  test_pdfout_fgrep (0, "GPL", "-V");
  test_pdfout_fgrep (0, "\nsetoutline\n", "-l");
  test_pdfout_fgrep (0, "Dump outline", "-d");
  
  /* invalid mode */
  test_pdfout_status (EX_USAGE, 0, 0, "modus");
  
  /* no arguments */
  test_pdfout_status (EX_USAGE, 0, 0);
  
  return 0;
}
