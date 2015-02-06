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


#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

static void print_escape (int c)
{
  printf ("\\%c", c);
}

static void quote_line (char *line, size_t len)
{
  char *end = line + len;
  
  printf ("\\\n");
  for (; line < end; ++line)
    {
      switch (*line)
	{
	case '\0':
	  printf (" \"\"\\0\"\" ");
	  break;

	case '"':
	  print_escape ('"');
	  break;

	case '\\':
	  print_escape ('\\');
	  break;
	  
	case '\a':
	  print_escape ('a');
	  break;
	  
	case '\b':
	  print_escape ('b');
	  break;
	  
	case '\f':
	  print_escape ('f');
	  break;
	  
	case '\n':
	  print_escape ('n');
	  break;
	  
	case '\r':
	  print_escape ('r');
	  break;
	  
	case '\t':
	  print_escape ('t');
	  break;
	  
	case '\v':
	  print_escape ('v');
	  break;
	  
	default:
	  putchar (*line);
	}
    }
}

int
main (int argc, char **argv)
{
  char *line = NULL;
  size_t line_len = 0;
  ssize_t read;
  putchar ('"');
  while ((read = getline (&line, &line_len, stdin)) != -1)
    {
      quote_line (line, read);
    }
  printf ("\"\n");
  return 0;
}
