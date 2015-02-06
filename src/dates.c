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


#include "common.h"

#include <regex.h>
#include <xalloc.h>
#include <xmemdup0.h>

/* We use posix Extended Regular Expressions as defined in
   <pubs.opengroup.org/onlinepubs/9699919799/>.

   The used GNU API is documented in
   www.gnu.org/software/gnulib/manual/html_node/GNU-Regex-Functions.html
*/

static struct re_pattern_buffer *pattern_buffer;
static struct re_registers *regs;


static void
setup_regexp (void)
{
  char *regex;
  const char *error_string;
  
  if (pattern_buffer)
    return;
  
  regex =
    "^(D:)?"	     /* optional prefix */
    "([0-9]{4})"     /* year */
    "(([0-9]{2})"    /* month */
    "(([0-9]{2})"    /* day */
    "(([0-9]{2})"    /* hour */
    "(([0-9]{2})"    /* minute */
    "(([0-9]{2})"    /* second */
    "([-+Z]"         /* relationship of local time to Universal Time (UT) */
    "(([0-9]{2})"    /* hour offset of UT */
    "('|'([0-9]{2})'?" /* minute offset to UT */
    ")?)?)?)?)?)?)?)?$";
  
  re_set_syntax (RE_SYNTAX_POSIX_EXTENDED);
  pattern_buffer = XZALLOC (struct re_pattern_buffer);
  error_string  = re_compile_pattern (regex, strlen (regex), pattern_buffer);
  if (error_string)
    error (1, 0, "re_compile_pattern: %s", error_string);

  regs = XZALLOC (struct re_registers);
}

#define MSG(fmt, args...) pdfout_msg ("check date string: " fmt, ## args)

static int
get_number (const char *string, int start, int end)
{
  char *buf = xmemdup0 (string + start, end - start);
  int number = pdfout_strtoint_null (buf);
  free (buf);
  return number;
}

/* subexpression index */
enum {
  PREFIX = 1,
  YEAR = PREFIX + 1,
  MONTH = YEAR + 2,
  DAY = MONTH + 2,
  HOUR = DAY + 2,
  MINUTE = HOUR + 2,
  SECOND = MINUTE + 2,
  HOUR_OFFSET = SECOND + 3,
  MINUTE_OFFSET = HOUR_OFFSET + 2,
};

int
pdfout_check_date_string (const char *date)
{
  int error_code, number;
  
  setup_regexp ();
  
  error_code = re_match (pattern_buffer, date, strlen (date), 0, regs);

  if (error_code == -2)
    error (1, errno, "re_match");

  if (error_code == -1)
    return -1;
  
  if (regs->start[MONTH] > 0)
    {
      number = get_number (date, regs->start[MONTH], regs->end[MONTH]);
      if (number < 1 || number > 12)
	{
	  MSG ("month %d out of range 01-12", number);
	  return -1;
	}
    }
  
  if (regs->start[DAY] > 0)
    {
      number = get_number (date, regs->start[DAY], regs->end[DAY]);
      if (number < 1 || number > 31)
  	{
  	  MSG ("day %d out of range 01-31", number);
  	  return -1;
  	}
    }

    if (regs->start[HOUR] > 0)
    {
      number = get_number (date, regs->start[HOUR], regs->end[HOUR]);
      if (number < 0 || number > 23)
  	{
  	  MSG ("hour %d out of range 00-23", number);
  	  return -1;
  	}
    }
    
  if (regs->start[MINUTE] > 0)
    {
      number = get_number (date, regs->start[MINUTE], regs->end[MINUTE]);
      if (number < 0 || number > 59)
  	{
  	  MSG ("minute %d out of range 00-59", number);
  	  return -1;
  	}
    }
  
  if (regs->start[SECOND] > 0)
    {
      number = get_number (date, regs->start[SECOND], regs->end[SECOND]);
      if (number < 0 || number > 59)
  	{
  	  MSG ("second %d out of range 00-59", number);
  	  return -1;
  	}
    }

  if (regs->start[HOUR_OFFSET] > 0)
    {
      number = get_number (date, regs->start[HOUR_OFFSET],
			   regs->end[HOUR_OFFSET]);
      if (number < 0 || number > 23)
	{
	  MSG ("hour offset to Universial Time '%d' out of range 00-23",
	       number);
	  return -1;
	}
    }
  
  if (regs->start[MINUTE] > 0)
    {
      number = get_number (date, regs->start[MINUTE_OFFSET],
			   regs->end[MINUTE_OFFSET]);
      if (number < 0 || number > 59)
  	{
  	  MSG ("minute offset to Universial Time '%d' out of range 00-59",
	       number);
  	  return -1;
  	}
    }
  return 0;
}
