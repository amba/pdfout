/* FIXME! header with FSF copyright */
#include <config.h>

#include "pdfout-regex.h"

#include <stdlib.h>
#include <string.h>
#include <error.h>
#include <errno.h>

#include <xalloc.h>
#include <xmemdup0.h>
#include <regex-quote.h>


const char *
pdfout_re_compile_pattern (const char *pattern, size_t length,
			   reg_syntax_t syntax, unsigned long options,
			   struct pdfout_re_pattern_buffer *buffer)
{
  const char *error_string;
  struct re_pattern_buffer *pattern_buffer = &buffer->pattern_buffer;
  char *tmp, *pattern0;
  
  pattern0 = xmemdup0 (pattern, length);
  if (options & PDFOUT_RE_FIXED_STRING)
    {
      struct regex_quote_spec spec;
      syntax = RE_SYNTAX_GREP;
      spec = regex_quote_spec_gnu (syntax, false);
      tmp = regex_quote (pattern0, &spec);
      free (pattern0);
      pattern0 = tmp;
      length = strlen (pattern0);
    }

  
  if (options & PDFOUT_RE_LINE_REGEXP)
    {
      /* Add anchors.  */
      static char const line_beg_no_bk[] = "^(";
      static char const line_end_no_bk[] = ")$";
      static char const line_beg_bk[] = "^\\(";
      static char const line_end_bk[] = "\\)$";
      int bk = !(syntax & RE_NO_BK_PARENS);
      size_t total;
      
      tmp = xcharalloc (sizeof line_beg_bk - 1 + length + sizeof line_end_bk);
      strcpy (tmp, bk ? line_beg_bk : line_beg_no_bk);
      total = strlen (tmp);
      memcpy (tmp + total, pattern0, length);
      total += length;
      strcpy (tmp + total, bk ? line_end_bk : line_end_no_bk);
      total += strlen (tmp + total);
      length = total;
      free (pattern0);
      pattern0 = tmp;
    }
  
  re_set_syntax (syntax);
  error_string = re_compile_pattern (pattern0, length, pattern_buffer);
  if (error_string)
    return error_string;

  /* Setup fastmap. */
  pattern_buffer->fastmap = xcharalloc (256);

  free (pattern0);
  return NULL;
}

regoff_t
pdfout_re_search (struct pdfout_re_pattern_buffer *buffer,
		  const char *string, __re_idx_t length,
		  __re_idx_t start, regoff_t range)
{
  regoff_t result = re_search (&buffer->pattern_buffer, string, length, start,
			       range, &buffer->regs);
  if (result >= 0)
    {
      buffer->start = (char *) string + buffer->regs.start[0];
      buffer->end = (char *) string + buffer->regs.end[0];
    }
  
  return result;
}

regoff_t
pdfout_re_match (struct pdfout_re_pattern_buffer *buffer,
		 const char *string, __re_idx_t length,
		 __re_idx_t start)
{
  regoff_t result = re_match (&buffer->pattern_buffer, string, length, start,
			      &buffer->regs);
  if (result >= 0)
    {
      buffer->start = (char *) string + buffer->regs.start[0];
      buffer->end = (char *) string + buffer->regs.end[0];
    }
  
  return result;
}

void
pdfout_re_free (struct pdfout_re_pattern_buffer *buffer)
{
  regfree (&buffer->pattern_buffer);
  free (buffer);
}
