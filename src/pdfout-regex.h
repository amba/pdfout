#ifndef HAVE_PDFOUT_REGEX_H
#define HAVE_PDFOUT_REGEX_H

#include <regex.h>
/* The underlying GNU Regex API is documented in
   www.gnu.org/software/gnulib/manual/html_node/GNU-Regex-Functions.html .  */


struct pdfout_re_pattern_buffer
{
  /* After a successfull match these point to start and end of the matched
     substring.  */
  const char *start;
  const char *end;
  
  unsigned long options;
  struct re_registers regs;
  struct re_pattern_buffer pattern_buffer;
};

/* Only match complete lines.  */
#define PDFOUT_RE_LINE_REGEXP ((unsigned long) 1)
/* Match fixed strings.  */
#define PDFOUT_RE_FIXED_STRING (PDFOUT_RE_LINE_REGEXP << 1)

/* Compile PATTERN into BUFFER.  OPTIONS is bitwise OR of PDFOUT_RE_*.  */
const char *
pdfout_re_compile_pattern (const char *pattern, size_t length,
			   reg_syntax_t syntax, unsigned long options,
			   struct pdfout_re_pattern_buffer *buffer);

regoff_t pdfout_re_search (struct pdfout_re_pattern_buffer *buffer,
			   const char *string, __re_idx_t length,
			   __re_idx_t start, regoff_t range);

regoff_t pdfout_re_match (struct pdfout_re_pattern_buffer *buffer,
			  const char *string, __re_idx_t length,
			  __re_idx_t start);

/* Free contents of BUFFER, and BUFFER itself.  */
void pdfout_re_free (struct pdfout_re_pattern_buffer *buffer);


#endif	/* !HAVE_PDFOUT_REGEX_H */
