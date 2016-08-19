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

static bool
streq(const char *a, const char *b)
{
  return strcmp (a, b) == 0;
}

int
strmatch (const char *key, const char *const *list)
{
  for (int i = 0; list[i]; ++i)
    {
      if (streq(key, list[i]))
	return i;
    }
  return -1;
}

char *
upcase (char *s)
{
  char *p;
  for (p = s; *p; ++p)
    *p = pdfout_toupper (*p);
  return s;
}

char *
lowercase (char *s)
{
  char *p;
  for (p = s; *p; ++p)
    *p = pdfout_tolower (*p);
  return s;
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


void
pdfout_argp_parse (const struct argp * argp, int argc, char ** argv,
		   unsigned flags, int *arg_index, void *input)
{
  error_t err;
  
  err = argp_parse (argp, argc, argv, flags, arg_index, input);
  if (err)
    {
      pdfout_errno_msg (err, "error: argp_parse");
      exit (1);
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

static FILE *
fopen_throw (fz_context *ctx, const char *filename, const char *mode)
{
  FILE *result = fopen (filename, mode);

  if (result == NULL)
      pdfout_throw_errno (ctx, "cannot open '%s'", filename);
  
  return result;
}

static char *
append_suffix (fz_context *ctx, const char *filename, const char *suffix)
{
  size_t filename_len = strlen (filename);
  size_t suffix_len = strlen (suffix);
  char *result = fz_malloc (ctx, filename_len + suffix_len + 1);
  
  memcpy (result, filename, filename_len);
  memcpy (result + filename_len, suffix, suffix_len + 1);
  return result;
}

static FILE *
open_default_file (fz_context *ctx, const char *filename,
		   const char *suffix, const char *mode)
{
  char *default_filename = append_suffix (ctx, filename, suffix);
  FILE *file = fopen_throw (ctx, default_filename, mode);
  if (streq (mode, "w"))
    pdfout_warn (ctx, "writing output to file '%s'", default_filename);
  free (default_filename);
  return file;
}

FILE *
open_default_read_file (fz_context *ctx, const char *filename,
			const char *suffix)
{
  return open_default_file (ctx, filename, suffix, "r");
}

FILE *
open_default_write_file (fz_context *ctx, const char *filename,
			 const char *suffix)
{
  return open_default_file (ctx, filename, suffix, "w");
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
  int result;
  
  result = strmatch (format, outline_format_list);

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

      number = pdfout_strtoint_old (number_token, &tailptr);
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
      number = pdfout_strtoint_old (range_token, &tailptr);
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
pdfout_parse_page_range (fz_context *ctx, const char *ranges, int page_count)
{
  int *result;
  int result_len = 2;
  /* find result len */
  const char *range_ptr;
  char *ranges_copy = fz_strdup (ctx, ranges);

  for (range_ptr = ranges; *range_ptr; ++range_ptr)
    {
      if (*range_ptr == ',')
	++result_len;
    }

  result = fz_malloc (ctx, 2 * result_len * sizeof (int));
  
  if (get_range (result, ranges_copy, page_count))
    exit (1);

  free (ranges_copy);
  return result;
}

