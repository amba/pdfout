/* The pdfout document modification and analysis tool.
   pdfout grep subcommand: GNU-style grep for PDF.
   Copyright (C) 1992, 1997-2002, 2004-2015 Free Software Foundation, Inc.
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


/* FIXME!!! comment on relation to GNU Grep sources, Mike Haertel */
/* FIXME! push branch to github? rebasing? */

#include "common.h"
#include "shared.h"
#include "pdfout-regex.h"
#include "../page-labels.h"
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>

#include <xalloc.h>
#include <exitfail.h>
#include <full-read.h>

#define SEP_CHAR_SELECTED ':'
#define SEP_CHAR_REJECTED '-'
#define SEP_STR_GROUP     "--"
#define SEP_STR_PAGE      "----"


static const char usage[] = "PATTERN PDF1 [PDF2...]";
static const char args_doc[] = "Search for PATTERN in each PDF.\n"
  "PATTERN is, by default, a basic regular expression (BRE).\n";

enum {
  EXIT_TROUBLE = 2,
  GROUP_SEPARATOR_OPTION = CHAR_MAX + 1,
  PAGE_SEPARATOR_OPTION,
  COLOR_OPTION,
  DEBUG_OPTION,
};

static struct argp_option options[] = {
  /* Use empty group headers, like {0, 0, 0, 0, ""}, to keep GNU Greps
     ordering in each group.  */
  {0, 0, 0, 0, "Regexp selection and interpretation:"},
  {"extended-regexp", 'E', 0, 0,
   "PATTERN is an extended regular expression (ERE)"},
  {"fixed-strings", 'F', 0, 0,
   "PATTERN is a set of newline-separated strings"},
  {"basic-regexp", 'G', 0, 0,
   "PATTERN is a basic regular expression (BRE)"},
  {0, 0, 0, 0, ""},
  {"regexp", 'e', "PATTERN", 0, "use PATTERN for matching"},
  {"file", 'f', "FILE", 0, "obtain PATTERN from FILE"},
  {"ignore-case", 'i', 0, 0, "ignore case distinctions"},
  {"line-regexp", 'x', 0, 0, "force PATTERN to match only whole lines"},
  /* FIXME: --word-regexp option.  */

  {0, 0, 0, 0, "Miscellaneous:"},
  {"no-messages", 's', 0, 0, "suppress error messages"},
  {"invert-match", 'v', 0, 0, "select non-matching lines"},

  {0, 0, 0, 0, "Output control:"},
  {"max-count", 'm', "NUM", 0, "stop after NUM matches"},
  {0, 0, 0, 0, ""},
  {"line-number", 'n', 0, 0, "print line number with output lines"},
  {"page-number", 'p', "LABEL|INDEX|BOTH", OPTION_ARG_OPTIONAL,
   "print page number with output lines"},
  {"Arguments:\n\
   LABEL: print page label (like i, ii, iii, 1, 2, 3, ..., A-1, ...).\n\
   INDEX: print page numbers, starting with 1.\n\
   BOTH: print both page label and page number.\n\
   default: LABEL.", 0, 0, OPTION_DOC},
  /* {"line-buffered", KEY_LINE_BUFFERED, 0, 0, "flush output on every line"}, */
  /* {0, 0, 0, 0, ""}, */
  {"with-filename", 'H', 0, 0, "print the file name for each match"},
  {0, 0, 0, 0, ""},
  {"no-filename", 'h', 0, 0, "suppress the file name prefix on output"},
  {"only-matching", 'o', 0, 0,
   "show only the part of a line matching PATTERN"},
  {"quiet", 'q', 0, 0, "suppress all normal output"},
  {"silent", 0, 0, OPTION_ALIAS},
  {0, 0, 0, 0, ""},
  {"files-without-match", 'L', 0, 0,
   "print only names of PDFs containing no match"},
  {"files-with-matches", 'l', 0, 0,
   "print only names of PDFs containing matches"},
  {0, 0, 0, 0, ""},
  {"count", 'c', 0, 0, "print only a count of matching lines per PDF"},
  {"initial-tab", 'T', 0, 0, "make tabs line up (if needed)"},
  {"null", 'Z', 0, 0, "print 0 byte after PDF name"},
  {0, 0, 0, 0, "Context control:"},
  {"before-context", 'B', "NUM", 0, "print NUM lines of leading context"},
  {0, 0, 0, 0, ""},
  {"after-context", 'A', "NUM", 0, "print NUM lines of trailing context"},
  {"context", 'C', "NUM", 0, "print NUM lines of output context"},
  /* FIXME: group-separator, page-separator */
  {"-NUM", 0, 0, OPTION_DOC, "same as --context=NUM"},
  {0, '0', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '1', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '2', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '3', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '4', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '5', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '6', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '7', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '8', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, '9', "NUM", OPTION_ARG_OPTIONAL | OPTION_HIDDEN},
  {0, 0, 0, 0, ""},
  {"group-separator", GROUP_SEPARATOR_OPTION, "SEP", 0,
   "print SEP instead of '--' between groups of lines"},
  {"page-separator", PAGE_SEPARATOR_OPTION, "SEP", 0,
   "print SEP instead of '----' between pages"},
  {"color", COLOR_OPTION, "WHEN", OPTION_ARG_OPTIONAL,
   "use markers to highlight the matching strings; "
   "WHEN is 'always', 'never', or 'auto'"},
  {"colour", 0, 0, OPTION_ALIAS},

  {"debug", DEBUG_OPTION, 0, OPTION_HIDDEN,
   "read a maximum of PIPE_BUF bytes of text from stdin; "
   "useful in tests to avoid the overhead of PDF parsing"},
  {0}
};
/* FIXME: always use intmax_t instead of int? */
/* FIXME: copy comments from GNU Grep */
static char **pdfs;
static char *keys;
static size_t keycc;
static reg_syntax_t syntax = RE_SYNTAX_GREP;
static unsigned long match_options;
static bool out_invert;
static bool out_quiet;		/* Suppress all normal output. */
static int out_file;		/* Print filenames. */
static bool count_matches;
static bool align_tabs;
static int list_files;
static bool out_line;
static bool out_page_label;
static bool out_page_index;
static bool with_filenames;
static bool no_filenames;
static bool only_matching;
static intmax_t max_count = INTMAX_MAX;
static bool exit_on_match;
static int filename_mask = ~0;	/* If zero, output nulls after filenames.  */
static int out_before;		/* Lines of leading context.  */
static int out_after;		/* Lines of trailing context.  */
static bool use_context;        /* Print group separators.  */
static const char *group_separator = SEP_STR_GROUP;
static const char *page_separator = SEP_STR_PAGE;
static ssize_t lastout;
static bool first_context_on_page; /* Avoid printing SEP_STR_GROUP before the
				      first context on each page.  */
static bool first_page;		/* Avoid printing SEP_STR_PAGE before the
				   first page.  */
static intmax_t outleft;	/* Maximum number of lines to be output.  */
static bool done_on_match;	/* Stop scanning PDF on first match.  */
static bool exit_on_match;	/* Exit on first match.  */
static const char *filename;	/* PDF filename   */
static int page_index;
static pdfout_page_labels_t *page_labels;
static int color_option;        /* If nonzero, use color markers.  */
static bool debug_mode;		/* Do not parse PDFs, but read text from
				   stdin.  */
struct line
{
  const char *begin;
  size_t len;
  /* Does it match the pattern?  */
  bool matching;
};

/* Line table of current page.  */
static size_t num_lines;
static struct line *lines;

/* See comment on color-choice in GNU grep source. We add page_num_color and
   remove byte_num_color.  */
static const char *selected_match_color = "01;31";	/* bold red */
static const char *context_match_color  = "01;31";	/* bold red */
static const char *filename_color = "35";	/* magenta */
static const char *line_num_color = "32";	/* green */
static const char *page_num_color = "01;34";	/* bold blue */
static const char *sep_color      = "36";	/* cyan */
static const char *selected_line_color = "";	/* default color pair */
static const char *context_line_color  = "";	/* default color pair */
/* See comment on use of "\33[K" in GNU grep source.  */
static bool erase_line_to_right = true;

/* SGR utility functions.  */
static void
pr_sgr_start (char const *s)
{
  if (*s)
    {
      if (erase_line_to_right)
	printf ("\33[%sm\33[K", s);
      else
	printf ("\33[%sm", s);
    }
}

static void
pr_sgr_end (char const *s)
{
  if (*s)
    {
      if (erase_line_to_right)
	fputs ("\33[m\33[K", stdout);
      else
	fputs ("\33[m", stdout);
    }
}

static void
pr_sgr_start_if (char const *s)
{
  if (color_option)
    pr_sgr_start (s);
}

static void
pr_sgr_end_if (char const *s)
{
  if (color_option)
    pr_sgr_end (s);
}

static void
color_cap_mt_fct (void)
{
  /* Our caller just set selected_match_color.  */
  context_match_color = selected_match_color;
}

static void
color_cap_rv_fct (void)
{
  /* By this point, it was 1 (or already -1).  */
  color_option = -1;  /* That's still != 0.  */
}

static void
color_cap_ne_fct (void)
{
  erase_line_to_right = false;
}

static int
should_colorize (void)
{
  char const *t = getenv ("TERM");
  return t && strcmp (t, "dumb") != 0;
}

/* For GREP_COLORS.  */
struct color_cap
{
  const char *name;
  const char **var;
  void (*fct) (void);
};

static const struct color_cap color_dict[] = {
  { "mt", &selected_match_color, color_cap_mt_fct }, /* both ms/mc */
  { "ms", &selected_match_color, NULL }, /* selected matched text */
  { "mc", &context_match_color,  NULL }, /* context matched text */
  { "fn", &filename_color,       NULL }, /* filename */
  { "ln", &line_num_color,       NULL }, /* line number */
  { "pn", &page_num_color,       NULL }, /* page number */
  { "se", &sep_color,            NULL }, /* separator */
  { "sl", &selected_line_color,  NULL }, /* selected lines */
  { "cx", &context_line_color,   NULL }, /* context lines */
  { "rv", NULL,                  color_cap_rv_fct }, /* -v reverses sl/cx */
  { "ne", NULL,                  color_cap_ne_fct }, /* no EL on SGR_* */
  { NULL, NULL,                  NULL }
};

/* Parse GREP_COLORS.  The default would look like:
     GREP_COLORS='ms=01;31:mc=01;31:sl=:cx=:fn=35:ln=32:pn=01;34:se=36'
   with boolean capabilities (ne and rv) unset (i.e., omitted).
   No character escaping is needed or supported.  */
static void
parse_grep_colors (void)
{
  const char *p;
  char *q;
  char *name;
  char *val;

  p = getenv ("GREP_COLORS"); /* Plural! */
  if (p == NULL || *p == '\0')
    return;

  /* Work off a writable copy.  */
  q = xstrdup (p);

  name = q;
  val = NULL;
  /* From now on, be well-formed or you're gone.  */
  for (;;)
    if (*q == ':' || *q == '\0')
      {
        char c = *q;
        struct color_cap const *cap;

        *q++ = '\0'; /* Terminate name or val.  */
        /* Empty name without val (empty cap)
         * won't match and will be ignored.  */
        for (cap = color_dict; cap->name; cap++)
          if (strcmp (cap->name, name) == 0)
            break;
        /* If name unknown, go on for forward compatibility.  */
        if (cap->var && val)
          *(cap->var) = val;
        if (cap->fct)
          cap->fct ();
        if (c == '\0')
          return;
        name = q;
        val = NULL;
      }
    else if (*q == '=')
      {
        if (q == name || val)
          return;
        *q++ = '\0'; /* Terminate name.  */
        val = q; /* Can be the empty string.  */
      }
    else if (val == NULL)
      q++; /* Accumulate name.  */
    else if (*q == ';' || (*q >= '0' && *q <= '9'))
      q++; /* Accumulate val.  Protect the terminal from being sent crap.  */
    else
      return;
}

static int
length_arg (const char *string)
{
  char *tailptr;
  int result;
  errno = 0;
  result = pdfout_strtoint (string, &tailptr);
  if (tailptr == string || *tailptr != '\0' || result < 0)
    error (EXIT_TROUBLE, errno, "invalid length argument: '%s'", string);
  return result;
}

static int
context_length_arg (const char *string)
{
  use_context = true;
  return length_arg (string);
}

static void
pattern_file (const char *arg)
{
  FILE *fp;
  size_t oldcc, keyalloc, cc;
  int fread_errno;
  
  if (strcmp (arg, "-") == 0)
    fp = stdin;
  else
    fp = fopen (arg, "r");
  if (fp == NULL)
    error (EXIT_TROUBLE, errno, "%s", arg);
  
  for (keyalloc = 1; keyalloc <= keycc + 1; keyalloc *= 2);
  
  keys = xrealloc (keys, keyalloc);
  oldcc = keycc;
  
  while ((cc = fread (keys + keycc, 1, keyalloc - 1 - keycc, fp)) != 0)
    {
      keycc += cc;
      if (keycc == keyalloc - 1)
	keys = x2nrealloc (keys, &keyalloc, sizeof *keys);
    }
  
  fread_errno = errno;
  if (ferror (fp))
    error (EXIT_TROUBLE, fread_errno, "%s", arg);

  if (fp != stdin)
    fclose (fp);

  /* Append final newline if file ended in non-newline. */
  if (oldcc != keycc && keys[keycc - 1] != '\n')
    keys[keycc++] = '\n';
}

static void
pattern_option (const char *arg)
{
  size_t cc = strlen (arg);
  /* '+ 1' for the trailing newline.
     The final trailing newline will be removed after all '-e' and '-f'
     options have been parsed.  */
  keys = xrealloc (keys, keycc + cc + 1);
  strcpy (keys + keycc, arg);
  keycc += cc;
  keys[keycc++] = '\n';
}

static bool
parse_color_option (const char *arg)
{
      if (arg)
	{
	  if (strcasecmp (arg, "always") == 0
	      || strcasecmp (arg, "yes") == 0
	      || strcasecmp (arg, "force") == 0)
	    color_option = 1;
	  else if (strcasecmp (arg, "never") == 0
		   || strcasecmp (arg, "no") == 0
		   || strcasecmp (arg, "none") == 0) 
	    color_option = 0;
	  else if (strcasecmp (arg, "auto") == 0
		   || strcasecmp (arg, "tty") == 0
		   || strcasecmp (arg, "if-tty") == 0)
	    color_option = 2;
	  else
	    return true;
	}
      else
	color_option = 2;
      
      return false;
}

static void
parse_page_number_option (char *arg)
{
  if (arg)
    {
      const char *const valid[] = {"label", "index", "both", NULL};
      ptrdiff_t result = argcasematch (arg, valid);
      if (result < 0)
	error (EXIT_TROUBLE, 0, "invalid argument: %s", arg);
      if (result == 0 || result == 2)
	out_page_label = true;
      if (result == 1 || result == 2)
	out_page_index = true;
    }
  else
    out_page_label = true;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static size_t args_num;
  static char **args;
  char *num_arg = NULL;
  
  if ('0' <= key && key <= '9')
    {
      /* -NUM option */
      num_arg = xasprintf ("%c%s", key, arg ? arg : "");
      key = 'C';
    }
  
  switch (key)
    {
    case 'E': syntax = RE_SYNTAX_EGREP; break;
    case 'F': match_options |= PDFOUT_RE_FIXED_STRING; break;
    case 'G': syntax = RE_SYNTAX_GREP; break;
    case 'e': pattern_option (arg); break;
    case 'f': pattern_file (arg); break;
    case 'i': syntax |= RE_ICASE; break;
    case 'x': match_options |= PDFOUT_RE_LINE_REGEXP; break;
    case 's': pdfout_batch_mode = true; break;
    case 'v': out_invert = true; break;
    case 'm': max_count = length_arg (arg); break;
    case 'n': out_line = true; break;
    case 'p': parse_page_number_option (arg); break;
    case 'H': with_filenames = true;  no_filenames = false; break;
    case 'h': with_filenames = false; no_filenames = true;  break;
    case 'o': only_matching = true; break;
    case 'q': exit_on_match = true; exit_failure = 0; break;
    case 'L': list_files = -1; break;
    case 'l': list_files =  1; break;
    case 'c': count_matches = true; break;
    case 'T': align_tabs = true; break;
    case 'Z': filename_mask = 0; break;
    case 'B': out_before = context_length_arg (arg); break;
    case 'A': out_after  = context_length_arg (arg); break;
    case 'C':
      out_before = out_after = context_length_arg (num_arg ? num_arg : arg);
      break;
    case GROUP_SEPARATOR_OPTION: group_separator = arg; break;
    case PAGE_SEPARATOR_OPTION: page_separator = arg; break;
    case DEBUG_OPTION: debug_mode = true; break;
    case COLOR_OPTION:
      if (parse_color_option (arg))
	argp_error (state, "invalid color argument: '%s'", arg);
      break;
    case ARGP_KEY_ARGS:
      args = state->argv + state->next;
      args_num = state->argc - state->next;
      break;
      
    case ARGP_KEY_END:
      if (color_option == 2)
	color_option = isatty (STDOUT_FILENO) && should_colorize ();
      /* POSIX says that -q overrides -l, which in turn overrides the
	 other output options.  */
      if (exit_on_match)
	list_files = 0;
      if (exit_on_match | list_files)
	{
	  count_matches = false;
	  done_on_match = true;
	}
      out_quiet = count_matches | done_on_match;
      
      if (keys)
	{
	  if (keycc == 0)
	    {
	      /* No keys were specified (e.g. -f /dev/null). Match nothing.  */
	      out_invert ^= true;
	      match_options &= ~PDFOUT_RE_LINE_REGEXP;
	    }
	  else
	    /* Strip trailing newline.  */
	    --keycc;
	  pdfs = args;
	}
      else if (args_num-- >= 1)
	{
	  /* Read pattern from command-line. A copy must be made in case of an
	     xrealloc() or free() later.  */
	  keycc = strlen (args[0]);
	  keys = xmemdup (args[0], keycc + 1);
	  pdfs = args + 1;
	}
      else
	argp_usage (state);

      if (!debug_mode && args_num < 1)
	/* Need at least one PDF.  */
	argp_usage (state);
      
      /* If there are >= 2 PDFs on the command-line, show filename for each
	 match, unless option --no-filename is given. */
      if ((args_num >= 2 && !no_filenames) || with_filenames)
	out_file = 1;

      if (max_count == 0)
	exit (EXIT_FAILURE);
      
      if (color_option)
	parse_grep_colors ();
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  free (num_arg);
  return 0;
}

static const struct argp_child children[] = {{&pdfout_general_argp},{0}};
static struct argp argp = {options, parse_opt, usage, args_doc, children};



static struct pdfout_re_pattern_buffer *pattern_buffer;

static void
print_page_label (void)
{
  static char *buffer;
  static size_t buffer_len;
  
  char *result = pdfout_page_labels_print (page_index, page_labels, buffer,
					   &buffer_len);
  if (result != buffer)
    {
      free (buffer);
      buffer = result;
    }

  pr_sgr_start_if (page_num_color);
  fputs (buffer, stdout);
  pr_sgr_end_if (page_num_color);
}

static void
print_page_index(void)
{
  pr_sgr_start_if (page_num_color);
  printf ("%d", page_index + 1);
  pr_sgr_end_if (page_num_color);
}
  
/* Print filename of current PDF.  */
static void
print_filename (void)
{
  pr_sgr_start_if (filename_color);
  fputs (filename, stdout);
  pr_sgr_end_if (filename_color);
}

/* Print a separator character.  */
static void
print_sep (char sep)
{
  pr_sgr_start_if (sep_color);
  putchar (sep);
  pr_sgr_end_if (sep_color);
}

/* Print filename, page label, page number, line.  */
static void
print_line_head (size_t line_number)
{
  bool pending_sep = false;
  char sep = lines[line_number].matching ^ out_invert
    ? SEP_CHAR_SELECTED : SEP_CHAR_REJECTED;
    
  /* FIXME: page numbers */
  if (out_file)
    {
      print_filename ();
      if (filename_mask)
	pending_sep = true;
      else
	putchar (0);
    }
  
  if (out_page_label)
    {
      if (pending_sep)
	print_sep (sep);
      print_page_label ();
      pending_sep = true;
    }
  
  if (out_page_index)
    {
      if (pending_sep)
	print_sep (sep);
      print_page_index ();
      pending_sep = true;
    }
  
  if (out_line)
    {
      if (pending_sep)
	print_sep (sep);
      pr_sgr_start_if (line_num_color);
      if (align_tabs)
	/* Do this to maximize the probability of alignment across lines.  */
	/* FIXME: format string: use z or j modifier.  */
	printf ("%4lu", line_number + 1);
      else
	printf ("%lu", line_number + 1);
      pr_sgr_end_if (line_num_color);
      pending_sep = true;
    }
  if (pending_sep)
    {
      if (align_tabs)
	fputs ("\t\b", stdout);
      print_sep (sep);
    }
}

/* Called if ether only_matching is true or matches should be colored. Cycle
   through all matches in a line. If only_matching is true, print each match on
   a separate line.  */
static void cycle_matches (size_t line_no, const char *line_color,
			   const char *match_color)
{
  struct line *line = &lines[line_no];
  const char *beg = line->begin;
  const char *cur = beg;
  const char *lim = cur + line->len;
  const char *mid = NULL;
  const char *b;
  size_t match_size;
  regoff_t match_offset;
  
  assert (line->matching);
  
  while (cur < lim
	 && ((match_offset = pdfout_re_search (pattern_buffer, beg, lim - beg,
					       cur - beg, lim - beg)) >= 0))
    {
      b = beg + match_offset;
      match_size = pattern_buffer->end - b;
      /* Avoid matching the empty line at the end of the buffer. */
      if (b == lim)
        break;
      
      /* Avoid hanging on grep --color "" foo */
      if (match_size == 0)
	{
	  /* Step one byte forward.  */
	  match_size = 1;
	  /* Record that we did not yet output the stepped-over byte.  */
	  if (!mid)
	    mid = cur;
	}
      else
	{
	  if (only_matching)
	    print_line_head (line_no);
	  else
	    {
	      /* Print leading, non-match part.  */
	      if (mid)
		{
		  cur = mid;
		  mid = NULL;
		}
	      pr_sgr_start (line_color);
	      fwrite (cur, 1, b - cur, stdout);
	    }
	  /* Print matching part.  */
	  pr_sgr_start_if (match_color);
	  fwrite (b, 1, match_size, stdout);
	  pr_sgr_end_if (match_color);
	  if (only_matching)
	    putchar ('\n');
	}
      cur = b + match_size;
    }

  /* Print non-matching trailing part. In GNU grep this is done in
     print_line_tail.  */
  if (!only_matching)
    {
      if (mid)
	cur = mid;
      pr_sgr_start (line_color);
      fwrite (cur, 1, lim - cur, stdout);
      pr_sgr_end (line_color);
      putchar ('\n');
    }
}

static void print_line (size_t line_no)
{
  struct line *line = &lines[line_no];
  bool matching = line->matching;
  bool selected = matching ^ out_invert; 
  const char *line_color, *match_color;

  if (!only_matching)
    print_line_head (line_no);

  if (color_option)
    {
      line_color = selected ^ (out_invert && (color_option < 0))
	? selected_line_color : context_line_color;
      match_color = selected ? selected_match_color : context_match_color;
    }
  else
    line_color = match_color = NULL;
  
  if (matching && (only_matching
		   || (color_option && (*line_color || *match_color))))
    cycle_matches (line_no, line_color, match_color);
  else if (!only_matching)
    {
      pr_sgr_start_if (line_color);
      fwrite (line->begin, 1, line->len, stdout);
      pr_sgr_end_if (line_color);
      putchar ('\n');
    }
  lastout = line_no;
  /* FIXME: check ferror (stdout).  */
}

static void print_line_with_context (size_t line_no)
{
  size_t i;
    
  /* Find first line of leading context. lastout can be -1, so cast to a signed
     type.  */
  if ((intmax_t) line_no <= lastout)
    i = lastout + 1;
  else
    /* lastout + 1 is >= 0, so the cast is ok.  */
    for (i = line_no; i > 0 && i > (size_t) (lastout + 1)
	   && i + out_before > line_no; --i);
  
  /* Print the group separator unless we are adjacent to the previous
     context.  */
  if (use_context && first_context_on_page == false
      && i > (size_t) (lastout + 1))
    {
      pr_sgr_start_if (sep_color);
      puts (group_separator);
      pr_sgr_end_if (sep_color);
    }

  /* Print leading context, selected line and trailing context. When outleft
     becomes 0, print trailing context up to the next match.  */
  for (; i < num_lines && i <= line_no + out_after; ++i)
    {
      if (lines[i].matching ^ out_invert)
	{
	  if (outleft)
	    --outleft;
	  else
	    break;
	}
      print_line (i);
    }
  first_context_on_page = false;
}

/* Only call if there is a match.  */
static void
output_lines (void)
{
  size_t i;
  intmax_t outleft0 = outleft;
  assert (num_lines);
  lastout = -1;
  /* Print the page separator unless this is the first page with a match.  */
  if (use_context && first_page == false)
    {
      pr_sgr_start_if (sep_color);
      puts (page_separator);
      pr_sgr_end_if (sep_color);
    }
  first_context_on_page = true;
  /* Make sure print_line_with_context is called, even if outleft is
     already 0. This ensures that the trailing context of the outleft-th match
     is fully printed. */
  for (i = 0; i < num_lines && outleft0; ++i)
    {
      if (lines[i].matching ^ out_invert)
	{
	  print_line_with_context (i);
	  --outleft0;
	}
    }
  first_page = false;
}
/* Get count of C in string TEXT.  */
static size_t get_char_num (const char *text, int c)
{
  size_t num = 0;
  const char *p = text - 1;
  
  while ((p = strchr (p + 1, c)))
	 ++num;
	 
  return num;
}


static struct line *
get_lines (const char *text)
{
  const char *line_start, *line_end;
  size_t i;
  struct line *line;
  
  num_lines = get_char_num (text, '\n') + 1;
  
  lines = XNMALLOC (num_lines, struct line);

  line_start = text;
  
  for (i = 0; i < num_lines; ++i)
    {
      line = &lines[i];
      line->begin = line_start;
      line_end = strchrnul (line_start, '\n');

      if (line_end - line_start > INT_MAX)
	error (EXIT_TROUBLE, 0, "line is too long");
      
      line->len = line_end - line_start ;
      
      line_start = line_end + 1;
    }
  
  return lines;
}

/* Return number of matching lines. */
static int
match_lines (void)
{
  size_t i;
  struct line *line;
  regoff_t error_code;
  int matching_lines = 0;

  /* Need one more than outleft matches to find the end of the trailing
     context of the last match.  */
  for (i = 0; i < num_lines && matching_lines <= outleft; ++i)
    {
      line = &lines[i];
      
      error_code = pdfout_re_search (pattern_buffer, line->begin, line->len, 0,
				     line->len);
      if (error_code == -2)
	error (EXIT_TROUBLE, errno, "internal error in pdfout_re_search");
      
      if (error_code >= 0)
	line->matching = true;
      else
	line->matching = false;
      if (line->matching ^ out_invert)
	{
	  if (matching_lines < outleft)
	    ++matching_lines;
	  if (done_on_match)
	    {
	      if (exit_on_match)
		exit (0);
	      break;
	    }
	}
    }
  return matching_lines;
}

static int grep_page (const char *text)
{
  int count;
  lines = get_lines (text);
  count = match_lines ();
  
  if (count && !out_quiet)
    output_lines ();
  
  free (lines);
  return count;
}

/* Return 0 if there was a match, and 1 otherwise.  */
static bool grep_pdf (fz_context *ctx)
{
  pdf_document *doc = NULL;	/* Set to NULL for -Wmaybe-uninitialized.  */
  int page_count;
  int count = 0;
  bool status;

  first_page = true;
  outleft = max_count;
  
  if (debug_mode)
    page_count = 1;
  else
    {
      doc = pdfout_pdf_open_document (ctx,filename);
      page_count = pdf_count_pages (ctx, doc);
      if (out_page_label)
	{
	  if (pdfout_page_labels_get (&page_labels, ctx, doc) == 2
	      || page_labels == NULL)
	    {
	      /* Error or empty page labels.  */
	      out_page_label = false;
	      out_page_index |= true;
	    }
	}
    }
  
  for (page_index = 0; page_index < page_count; ++page_index)
    {
      char *text;
      size_t text_len;
      FILE *memstream;

      if (debug_mode)
	{
	  /* Read text from stdin.  */
	  text = xcharalloc (PIPE_BUF + 1);
	  errno = 0;
	  text_len = full_read (STDIN_FILENO, text, PIPE_BUF);
	  text[text_len] = 0;
	  if (text_len == PIPE_BUF)
	    error (99, 0, "grep_page: full_read exceeds PIPE_BUF bytes");
	  if (errno)
	    error (99, errno, "grep_page: full_read");
	}
      else
	{
	  memstream = open_memstream (&text, &text_len);
	  if (memstream == NULL)
	    error (EXIT_TROUBLE, errno, "open_memstream");
      
	  pdfout_text_get_page (memstream, ctx, doc, page_index);

	  if (fclose (memstream))
	    error (EXIT_TROUBLE, errno, "fclose");
	}

      /* Chomp trailing newline.  */
      if (text_len > 0 && text[text_len - 1] == '\n')
	text[--text_len] = 0;
      
      count += grep_page (text);

      free (text);
      
      if (!outleft || (done_on_match && count))
	break;
    }
  
  if (count_matches)
    {
      if (out_file)
	{
	  print_filename ();
	  if (filename_mask)
	    print_sep (SEP_CHAR_SELECTED);
	  else
	    putchar (0);
	}
      printf ("%d\n", count);
    }
  
  status = !count;
  if (list_files == 1 - 2 * status)
    {
      print_filename ();
      putchar ('\n' & filename_mask);
    }
  return status;
}

static void setup_regexp (void)
{
  const char *problem;
  
  if (pattern_buffer)
    /* already setup */
    return;

  pattern_buffer = XZALLOC (struct pdfout_re_pattern_buffer);
  problem = pdfout_re_compile_pattern (keys, keycc, syntax, match_options,
				       pattern_buffer);
  
  if (problem)
    error (EXIT_TROUBLE, errno, "%s", problem);
}

void
pdfout_command_grep (int argc, char **argv)
{
  bool status;
  fz_context *ctx = NULL;

  argp_err_exit_status = EXIT_TROUBLE;
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  setup_regexp ();
  status = 1;
  
  if (debug_mode)
    {
      filename = "stdin";
      status &= grep_pdf (NULL);
    }
  else
    {
      if (debug_mode == false)
	ctx = pdfout_new_context ();
      for (; *pdfs; ++pdfs)
	{
	  filename = *pdfs;
	  status &= grep_pdf (ctx);
	}
    }
  
  exit (status);
}
