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
#include "data/gettxt.h"

int
main (void)
{
  test_init ("gettxt-yaml");

  char *pdf = test_data ("hello-world.pdf");
  /* get lines */
  {
    test_pdfout (0, HELLO_WORLD_YAML_LINES, "gettxt", pdf, "-y");
  }

  /* get spans */
  {
    test_pdfout (0, HELLO_WORLD_YAML_SPANS, "gettxt", pdf, "-ys");
  }
  
  return 0;
}
