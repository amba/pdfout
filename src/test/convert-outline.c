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
main (void)
{
  test_init ();
  const char YAML[] = "\
-   title: title1\n\
    page: 1\n\
-   title: ζβσ\n\
    page: 2\n\
-   title: 3\n\
    page: 3\n\
    kids:\n\
    -   title: 4\n\
        page: 4\n\
    -   title: 5\n\
        page: 5\n\
        kids:\n\
        -   title: 6\n\
            page: 6\n\
        -   title: 7\n\
            page: 7\n\
            kids:\n\
            -   title: 8\n\
                page: 8\n\
    -   title: 9\n\
        page: 10\n\
-   title: 'empty titles:'\n\
    page: 10\n\
-   title: \n\
    page: 10\n\
    kids:\n\
    -   title: \n\
        page: 10\n";

  {
    /* convert to YAML and back */
    test_pdfout (INPUT, YAML, "convert-outline", "-fw");
    test_pdfout (YAML, EXPECTED, "convert-outline", "-tw");

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
