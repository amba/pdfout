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
main (int arg, char **argv)
{
  test_init ("outline-wysiwyg");

#define COMMAND "outline" , "-fw"

  
#define SUFFIX ".outline.wysiwyg"
#define BROKEN_INPUT 							\
  "title 11",			/* page number excceedes page count  */ \
    "title 0",			/* page number is 0 */			\
    "abc\xff\xff 1", 		/* broken UTF-8 */			\
    "1",			/* neither title nor separator */	\
    "title1",			/* no separator */			\
    "  d=1  .",			/* dot after offset marker */		\
    "title 1  .",		/* dot after page number */		\
    "title 2147483648",		/* INT_MAX overflow */			\
    "    abc 1", 		/* too mutch indent */			\
    "abc 1\n        def 2"	/* too mutch indent in line 2 */
  
#include "set-get-template.c"
  
  return 0;
}
