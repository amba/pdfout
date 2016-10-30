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

fz_context *
pdfout_new_context (void)
{
  fz_context *ctx = fz_new_context (NULL, NULL, FZ_STORE_DEFAULT);

  if (ctx == NULL)
    {
      fprintf (stderr, "cannot create context");
      exit (1);
    }
  
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

#define MSG(ctx, fmt, args...)					\
  pdfout_warn (ctx, "parsing page range: " fmt, ## args)

static int
get_range (fz_context *ctx, int *result, char *ranges, int page_count)
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
	  MSG (ctx, "empty page number");
	  return 1;
	}

      number = pdfout_strtoint(ctx, number_token, &tailptr);
      if (tailptr[0] != '\0')
	{
	  MSG (ctx, "not part of an integer: '%s'", tailptr);
	  return 1;
	}

      if (number < 1)
	{
	  MSG (ctx, "page %d is not positive", number);
	  return 1;
	}
      
      if (number > page_count)
      	{
	  MSG (ctx, "%d is greater than page count %d", number, page_count);
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
      number = pdfout_strtoint (ctx, range_token, &tailptr);
      if (tailptr[0] != '\0')
	{
	  MSG (ctx, "not part of an integer: '%s'", tailptr);
	  return 1;
	}
      
      if (number < 1)
	{
	  MSG (ctx, "page %d is not positive", number);
	  return 1;
	}
      
      if (number > page_count)
	{
	  MSG (ctx, "%d is greater than page count %d", number, page_count);
	  return 1;
	}
      
      if (number < result[pos])
	{
	  MSG (ctx, "%d is smaller than %d", number, result[pos]);
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
  
  if (get_range (ctx, result, ranges_copy, page_count))
    exit (1);

  free (ranges_copy);
  return result;
}

