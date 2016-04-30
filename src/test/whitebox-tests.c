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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int
main (void)
{
  test_init ();

  test_pdfout (0, 0, "debug", "--incremental-update");

  test_pdfout (0, 0, "debug", "--incremental-update-xref");

  test_pdfout (0, 0, "debug", "--string-conversions");

  test_pdfout (0, 0, "debug", "--regex");

  test_pdfout (0, 0, "debug", "--json");
  
  test_assert (test_streq (pdfout_append_suffix ("abc", ".yaml"), "abc.yaml"));
  
  return 0;
}
