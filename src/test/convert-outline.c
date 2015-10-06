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

/* define INPUT and EXPECTED */
#include "data/outline-wysiwyg.h"

int
main (int argc, char **argv)
{
  test_init ();

  {
    char *outline_yaml = test_tempname ();
    
    /* convert to YAML and back */
    test_pdfout (INPUT, 0, "convert-outline", "-fw", "-o", outline_yaml);
    test_pdfout (0, EXPECTED, "convert-outline", "-tw", outline_yaml);

    test_pdfout_status (EX_USAGE, 0, 0, "convert-outline", "-f", "lala");
  }

  /* YAML emitter options */
  test_pdfout ("ä ℕ 𝓓 a a a a a a a a a a a a a a a 1", "\
- title: \"\\xE4\n\
    \\u2115 \\U0001D4D3\n\
    a a a a\n\
    a a a a\n\
    a a a a\n\
    a a a\"\n\
  page: 1\n", "convert-outline" , "-e", "-i2", "-w10", "-fw");

  /* invalid indentation increment */
  test_pdfout_status (EX_USAGE, "1 1", 0, "convert-outline", "-i1", "-fw");
  
  /* check return value for broken input */
  test_pdfout_status (EX_DATAERR, "aaa1", 0,  "convert-outline", "-fw");
  
  return 0;
}
