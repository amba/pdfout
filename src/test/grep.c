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
#include <stdlib.h>
#include <xalloc.h>

static char *
slash2nl (const char *s)
{
  char *copy, *p;

  /* Work off a writable copy.  */
  copy = xstrdup (s);
  for (p = copy; *p; ++p)
    if (*p == '/')
      *p = '\n';
  
  return copy;
}

int
main (void)
{
  test_init ();

  /* Test page separator. FIXME: page numbers, page labels.  */
  {
    char *pdf = test_data ("hello-world.pdf");
    test_pdfout (0, "\
i:2:Hello, World from page 2!\n\
----\n\
A:3:Hello, World from page 3!\n" , "grep", "-C0", "-pboth", "2\\|3", pdf);
  }




  
  /* The following tests are based on GNU grep's tests/foad1 source file,
     which was started by Julian Foad.
  
     Test various combinations of command-line options.
   
     Copyright (C) 2001, 2006, 2009-2015 Free Software Foundation, Inc.
     Copying and distribution of this file, with or without modification,
     are permitted in any medium without royalty provided the copyright
     notice and this notice are preserved.
  */
  
#define grep_test(input, expected, pattern...)				\
  do									\
    {									\
      char *input_slash2nl = slash2nl (input);				\
      char *expected_slash2nl = slash2nl (expected);			\
									\
      test_pdfout (input_slash2nl, expected_slash2nl, "grep", "--debug", \
		   ## pattern);						\
									\
      free (input_slash2nl);						\
      free (expected_slash2nl);						\
    }									\
  while (0)

  /* Test "--only-matching" ("-o") option.  "-o" with "-i" should output an
     exact copy of the matching input text.  */
  grep_test ("WordA/wordB/WORDC/", "Word/word/WORD/", "word", "-oi");
  grep_test ("WordA/wordB/WORDC/", "Word/word/WORD/", "Word", "-oi");
  grep_test ("WordA/wordB/WORDC/", "Word/word/WORD/", "WORD", "-oi");

  /* Should display the line number (-n), or file name (-H) of every match, not
     just of the first match on each input line.  */
  grep_test ("wA wB/wC/", "1:wA/1:wB/2:wC/", "w.", "-on");
  grep_test ("wA wB/wC/", "1:wA/1:wB/2:wC/", "w.", "-on3");
  grep_test ("XwA YwB/ZwC/", "wA/wB/wC/", "w.", "-o");
  grep_test ("wA wB/", "stdin:wA/stdin:wB/", "w.", "-oH");
  grep_test ("wA wB/", "stdin:wA/stdin:wB/", "w.", "-oH3");

  /* Combination of -h and -H.  */
  grep_test ("wA wB/", "wA wB/", "w.", "-h");
  grep_test ("wA wB/", "wA wB/", "w.", "-Hh");
  grep_test ("wA wB/", "stdin:wA wB/", "w.", "-H");
  grep_test ("wA wB/", "stdin:wA wB/", "w.", "-hH");

  /*  */
  /* Test "--color" option.  */
  
# define CB "[01;31m[K"
# define CE "[m[K"

  /* "--color" with "-i" should output an exact copy of the matching input
     text. FIXME: filename, line number, page number, separators.  */
  grep_test ("WordA/wordb/WORDC/", CB"Word"CE"A/"CB"word"CE"b/"CB"WORD"CE"C/",
    "word", "--color=always", "-i");
  grep_test ("a bb ccc dddd/", "a "CB"bb"CE" "CB"ccc"CE" dddd/", "-ebb",
	     "-ecc", "-eccc", "--color=always");

  /* Test combination of "-m" with "-A".  */
  grep_test ("4/40/", "4/40/", "^4$", "-m1", "-A99");
  grep_test ("4/40/", "4/", "^4", "-m1", "-A99");

  return 0;
}
