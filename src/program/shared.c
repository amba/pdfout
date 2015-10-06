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


#include "shared.h"
#include <c-ctype.h>
#include <argmatch.h>

char *
upcase (char *s)
{
  char *p;
  for (p = s; *p; ++p)
    *p = c_toupper (*p);
  return s;
}

char *
lowercase (char *s)
{
  char *p;
  for (p = s; *p; ++p)
    *p = c_tolower (*p);
  return s;
}

ptrdiff_t
argcasematch (char *arg, const char *const *valid)
{
  return argmatch (lowercase (arg), valid, NULL, 0);
}

/* general options */

static struct argp_option general_options[] = {
  {0, 0, 0, 0, "general options:", -1},
  {"batch-mode", 'b', 0, 0, "do not print any status messages"},
  {0}
};

static error_t
general_parse_opt (int key, _GL_UNUSED char *arg,
		   _GL_UNUSED struct argp_state *state)
{
  switch (key)
    {
    case 'b':
      pdfout_batch_mode = 1;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp pdfout_general_argp = {general_options, general_parse_opt};


/* pdf output options */

static struct argp_option pdf_output_options[] = {
  {0, 0, 0, 0, "pdf output options:"},

  /* FIXME: option to update producer */
  /* {"no-info", PDFOUT_NO_INFO, 0, 0, */
  /*  "do not update the 'Producer' key in the information dictionary."}, */
  /* {0, 0, 0, 0, "options controlling encoding:"}, */
  {0}
};

static error_t
pdf_output_parse_opt (int key, _GL_UNUSED char *arg,
		      _GL_UNUSED struct argp_state *state)
{
  switch (key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

struct argp pdfout_pdf_output_argp = {
  pdf_output_options, pdf_output_parse_opt
};

/* YAML emitter options */

static struct argp_option yaml_emitter_options[] = {
  {0, 0, 0, 0, "YAML emitter options:"},
  {"yaml-indent", 'i', "[1-10]", 0,
   "indentation increment (default: 4)"},
  {"yaml-line-width", 'w', "WIDTH", 0,
   "preferred line width (default: unlimited)"},
  {"yaml-escape-unicode", 'e', 0, 0,
   "use universal character names \\xHH, \\uHHHH and \\UHHHHHHHH"
   " for non-ASCII Unicode codepoints"},
  {0}
};


extern int yaml_emitter_indent;
extern int yaml_emitter_line_width;
extern bool yaml_emitter_escape_unicode;

static error_t
yaml_emitter_parse_opt (int key, char *arg, struct argp_state *state)
{
  int i;
  switch (key) {
  case 'i':
    i = pdfout_strtoint_null (arg);
    if (i <= 1 || i >= 10)
      argp_error (state, "value of '--yaml-indent' must be in range 2-10");
    yaml_emitter_indent = i;
    break;
    
  case 'w':
    yaml_emitter_line_width = pdfout_strtoint_null (arg);
    break;
    
  case 'e':
    yaml_emitter_escape_unicode = true;
    break;
    
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp pdfout_yaml_emitter_argp =
  {yaml_emitter_options, yaml_emitter_parse_opt};

FILE *
pdfout_xfopen (const char *path, const char *mode)
{
  FILE *result = fopen (path, mode);
  if (result == NULL)
    {
      const char *mode_string = strcmp (mode, "r") == 0 ? "for reading" :
	strcmp (mode, "w") == 0 ? "for writing" :
	xasprintf ("(mode '%s')", mode);
      pdfout_errno_msg (errno, "cannot open file '%s' %s", path, mode_string);
      exit (EX_USAGE);
    }
  
  return result;
}

void
pdfout_argp_parse (const struct argp * argp, int argc, char ** argv,
		   unsigned flags, int *arg_index, void *input)
{
  error_t err;
  
  err = argp_parse (argp, argc, argv, flags, arg_index, input);
  if (err)
    {
      pdfout_errno_msg (err, "error: argp_parse");
      exit (EX_USAGE);
    }
}

fz_context *
pdfout_new_context (void)
{
  fz_context *ctx = fz_new_context (NULL, NULL, FZ_STORE_DEFAULT);

  if (ctx == NULL)
    error (1, errno, "cannot create context");

  return ctx;
}

FILE *
pdfout_get_stream (char **filename, char mode,
		   const char *pdf_filename,
		   bool use_default_filename, const char *suffix)
{
  FILE *result = NULL;

  assert (mode == 'w' || mode == 'r');
  
  if (*filename == NULL)
    {
      if (use_default_filename)
	{
	  *filename =
	    pdfout_append_suffix (pdf_filename, suffix);
	}
      else
	{
	  if (mode == 'r')
	    result = stdin;
	  else
	    result = stdout;
	}
    }

  if (result == NULL)
    result = pdfout_xfopen (*filename, mode == 'w' ? "w" : "r");
  
  return result;
}



static const char *const outline_format_list[] = {"yaml", "wysiwyg", NULL};

static void list_formats (FILE *stream)
{
  const char *const *list = &outline_format_list[1];

  fprintf (stream, "known formats: %s", outline_format_list[0]);
  
      for (; *list; ++list)
	fprintf (stream, ", %s", *list);

      fprintf (stream, "\n");
}

enum pdfout_outline_format
pdfout_outline_get_format (struct argp_state *state, char *format)
{
  ptrdiff_t result;
  
  result = argcasematch (format, outline_format_list);

  if (result >= 0)
    return result;
  
  if (result == -1)
    fprintf (state->err_stream, "%s: unknown outline format: '%s'\n",
	     state->name, format);

  else if (result == -2)
    fprintf (state->err_stream, "%s: ambiguous outline format: '%s'\n",
	     state->name, format);

  fprintf (state->err_stream, "%s: ", state->name);
  list_formats (state->err_stream);
  exit (argp_err_exit_status);
}

#define MSG(fmt, args...) \
  pdfout_msg ("parsing page range: " fmt, ## args)

static int
get_range (int *result, char *ranges, int page_count)
{
  char *range_token;
  int pos = 0;
  
  /* strsep is documented in
   www.gnu.org/software/libc/manual/html_node/Finding-Tokens-in-a-String.html
  */
  
  for (range_token = strsep (&ranges, ","); range_token;
       range_token = strsep (&ranges, ","), pos += 2)
    {
      char *tailptr, *number_token = strsep (&range_token, "-");
      int number;

      if (number_token[0] == '\0')
	{
	  MSG ("empty page number");
	  return 1;
	}

      number = pdfout_strtoint (number_token, &tailptr);
      if (tailptr[0] != '\0')
	{
	  MSG ("not part of an integer: '%s'", tailptr);
	  return 1;
	}

      if (number < 1)
	{
	  MSG ("page %d is not positive", number);
	  return 1;
	}
      
      if (number > page_count)
      	{
	  MSG ("%d is greater than page count %d", number, page_count);
	  return 1;
	}
      
      result[pos] = number;

      if (range_token == NULL)
	{
	  /* no hyphen => last = first */
	  result[pos + 1] = number;
	  continue;
	}
      
      /* parse second number after the hyphen*/
      number = pdfout_strtoint (range_token, &tailptr);
      if (tailptr[0] != '\0')
	{
	  MSG ("not part of an integer: '%s'", tailptr);
	  return 1;
	}
      
      if (number < 1)
	{
	  MSG ("page %d is not positive", number);
	  return 1;
	}
      
      if (number > page_count)
	{
	  MSG ("%d is greater than page count %d", number, page_count);
	  return 1;
	}
      
      if (number < result[pos])
	{
	  MSG ("%d is smaller than %d", number, result[pos]);
	  return 1;
	}
      
      result[pos + 1] = number;
    }
  
  result[pos] = 0;
  result[pos + 1] = 0;
  return 0;
}

int *
pdfout_parse_page_range (const char *ranges, int page_count)
{
  int *result;
  int result_len = 2;
  /* find result len */
  const char *range_ptr;
  char *ranges_copy = xstrdup (ranges);

  for (range_ptr = ranges; *range_ptr; ++range_ptr)
    {
      if (*range_ptr == ',')
	++result_len;
    }

  result = XNMALLOC (2 * result_len, int);
  
  if (get_range (result, ranges_copy, page_count))
    exit (EX_USAGE);

  free (ranges_copy);
  return result;
}

