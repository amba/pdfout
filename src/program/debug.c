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
#include "shared.h"
#include "tmpdir.h"
#include "tempname.h"
#include "charset-conversion.h"
#include "pdfout-regex.h"

static char usage[] = "";
static char doc[] = "Run checks, called by make check\v";

enum {
  INCREMENTAL_UPDATE = CHAR_MAX + 1,
  INCREMENTAL_UPDATE_XREF,
  STRING_CONVERSIONS,
  REGEX,
};

static struct argp_option options[] = {
  {"incremental-update", INCREMENTAL_UPDATE},
  {"incremental-update-xref", INCREMENTAL_UPDATE_XREF},
  {"string-conversions", STRING_CONVERSIONS},
  {"regex", REGEX},
  {0}
};




#define test_assert(expr)					\
  do								\
    {								\
      if (!(expr))						\
	{							\
	  fprintf (stderr, "%s:%d: assertion '%s' failed\n",	\
		   __FILE__, __LINE__, #expr);			\
	  exit (1);					       	\
	}							\
    }								\
  while (0)

/* create filename in a temporary directory */
static char *
temp_filename (void)
{
  int file_len = 256;
  char *file = XNMALLOC (file_len, char);
    
  if (path_search (file, file_len, NULL, NULL, true))
    error (1, 0, "temp_filename: path_search");
  if (gen_tempname (file, 0, 0, GT_NOCREATE))
    error (1, 0, "temp_filename: gen_tempname");

  return file;
}

static pdf_document *
check_open (fz_context * ctx, char *file)
{
  pdf_document *doc = pdf_open_document (ctx, file);
  if (doc->repair_attempted || doc->freeze_updates)
    error (1, 0, "pdf_open_document: broken_pdf: %s", file);
  return doc;
}

static void
check_incremental_update (void)
{
  pdf_write_options opts = { 0 };
  fz_context *ctx;
  /* to run Memcheck-clean, keep a pointer to each doc  */
  pdf_document *doc[3];
  pdf_obj *info;
  char *file;
  const char *subject = "check incremental update";
  int num;

  ctx = pdfout_new_context ();

  doc[0] = pdf_create_document (ctx);

  file = temp_filename ();

  pdfout_msg ("file: %s", file);
  
  pdf_save_document (ctx, doc[0], file, &opts);

  doc[1] = check_open (ctx, file);

  num = pdf_create_object (ctx, doc[1]);
  info = pdf_new_dict (ctx, doc[1], 1);
  pdf_update_object (ctx, doc[1], num, info);
  pdf_drop_obj (ctx, info);

  pdf_dict_puts_drop (ctx, info, "Subject",
		      pdf_new_string (ctx, doc[1], subject, strlen (subject)));
  pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc[1]), "Info",
		      pdf_new_indirect (ctx, doc[1], num, 0));

  opts.do_incremental = 1;
  pdf_save_document (ctx, doc[1], file, &opts);

  doc[2] = check_open (ctx, file);

  if (unlink (file))
    error (1, errno, "error unlinking '%s'", file);

  exit (0);
}

/* ------------------------  */
/* same with xref streams    */
/* see Mupdf Bug 695892      */
/* ------------------------  */
static void
check_incremental_update_xref (void)
{
  pdf_write_options opts = { 0 };
  fz_context *ctx;
  /* to run Memcheck-clean, keep a pointer to each doc  */
  pdf_document *doc[3];
  pdf_obj *info;
  char *file;
  const char *subject = "check incremental update with xref";
  int num;

  ctx = pdfout_new_context ();
  
  file = temp_filename ();
  pdfout_msg ("file: %s", file);
  doc[0] = pdf_create_document (ctx);

  opts.do_incremental = 0;
  pdf_save_document (ctx, doc[0], file, &opts);

  doc[1] = check_open (ctx, file);

  /* ensure that incremental update will have xref stream */
  doc[1]->has_xref_streams = 1;

  num = pdf_create_object (ctx, doc[1]);
  info = pdf_new_dict (ctx, doc[1], 1);
  pdf_update_object (ctx, doc[1], num, info);
  pdf_drop_obj (ctx, info);

  pdf_dict_puts_drop (ctx, info, "Subject",
		      pdf_new_string (ctx, doc[1], subject, strlen (subject)));
  pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc[1]), "Info",
		      pdf_new_indirect (ctx, doc[1], num, 0));


  opts.do_incremental = 1;
  pdf_save_document (ctx, doc[1], file, &opts);

  doc[2] = check_open (ctx, file);

  if (unlink (file))
    error (1, errno, "error unlinking '%s'", file);
  
  exit (0);
}

static void print_string (const char *string, size_t len)
{
  fprintf (stderr, "length: %zu, string:\n", len);
  for (size_t i = 0; i < len; ++i)
    fprintf (stderr, " %02x", (unsigned char) string[i]);
  fprintf (stderr, "\n");
}

#define test_equal(result, expected, result_len, expected_len)	\
  do								\
    {								\
      if (result_len != expected_len				\
	  || memcmp (result, expected, result_len))		\
	{							\
	  fprintf (stderr, "expected:");			\
	  print_string (expected, expected_len);		\
	  fprintf (stderr, "got:");				\
	  print_string (result, result_len);			\
	  fprintf (stderr, "at file: %s, line: %d\n",		\
		   __FILE__, __LINE__);				\
	  exit(1);						\
	}							\
    } while (0)

/* Check that from -> too -> from is the identity.  */
#define test_conversion(from, too, src, expected)			\
  do									\
    {									\
      size_t buffer_len, buffer_len_back;				\
      char *result, *result_back;					\
      result = pdfout_char_conv (ctx, from, too, src, sizeof src - 1, \
				 NULL, &buffer_len);			\
      test_equal (result, expected, buffer_len, sizeof expected - 1);	\
      result_back = pdfout_char_conv (ctx, too, from, result, buffer_len, \
				      NULL, &buffer_len_back);		\
      test_equal (result_back, src, buffer_len_back, sizeof src - 1);	\
      free (result);							\
      free (result_back);						\
    } while (0);

#define test_from_utf8(too, src, expected)	\
  test_conversion ("UTF-8", too, src, expected)

#define test_pdf_from_utf8(src, expected)				\
  do									\
    {									\
      size_t buffer_len, buffer_len_back;				\
      char *result, *result_back;					\
      result = pdfout_utf8_to_pdf (ctx, src, sizeof src - 1, &buffer_len); \
      test_equal (result, expected, buffer_len, sizeof expected - 1);	\
      result_back = pdfout_pdf_to_utf8 (ctx, result, buffer_len,	\
					&buffer_len_back);		\
      test_equal (result_back, src, buffer_len_back, sizeof src - 1);	\
      free (result);							\
      free (result_back);						\
    } while (0);

static void
check_string_conversions (void)
{
  fz_context *ctx = pdfout_new_context();

  /* FIXME: check trailing zero for 'C' encoding. */
  /* FIXME: check with just enough space and just too little space.  */
  
  test_from_utf8 ("ASCII", "0123456789", "0123456789");
  test_from_utf8 ("UTF-8", "0123456789", "0123456789");
  test_from_utf8 ("C", "0123456789", "0123456789");
  test_from_utf8 ("UTF-16", "abc", "\xfe\xff\0a\0b\0c");
  test_from_utf8 ("UTF-16", "αβγ", "\xfe\xff\x03\xb1\x03\xb2\x03\xb3");
  test_pdf_from_utf8 ("abc", "abc");
  test_pdf_from_utf8 ("αβγ", "\xfe\xff\x03\xb1\x03\xb2\x03\xb3");
  test_pdf_from_utf8 ("\U0001D4D3", "\xfe\xff\xd8\x35\xdc\xd3");
  test_from_utf8 ("UTF-16BE", "αβγ", "\x03\xb1\x03\xb2\x03\xb3");
  test_from_utf8 ("UTF-16LE", "αβγ", "\xb1\x03\xb2\x03\xb3\x03");
  test_from_utf8 ("UTF-32LE", "αβγ",
		  "\xb1\x03\x00\x00" "\xb2\x03\x00\x00" "\xb3\x03\x00\x00");
  test_from_utf8 ("UTF-32BE", "αβγ",
		  "\x00\x00\x03\xb1" "\x00\x00\x03\xb2" "\x00\x00\x03\xb3");
  test_from_utf8 ("UTF-32", "αβγ", "\x00\x00\xfe\xff" "\x00\x00\x03\xb1"
		  "\x00\x00\x03\xb2" "\x00\x00\x03\xb3");

  test_from_utf8 ("PDFDOC", "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\xcb\x98\xcb\x87\xcb\x86\xcb\x99\xcb\x9d\xcb\x9b\xcb\x9a\xcb\x9c\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\xe2\x80\xa2\xe2\x80\xa0\xe2\x80\xa1\xe2\x80\xa6\xe2\x80\x94\xe2\x80\x93\xc6\x92\xe2\x81\x84\xe2\x80\xb9\xe2\x80\xba\xe2\x88\x92\xe2\x80\xb0\xe2\x80\x9e\xe2\x80\x9c\xe2\x80\x9d\xe2\x80\x98\xe2\x80\x99\xe2\x80\x9a\xe2\x84\xa2\xef\xac\x81\xef\xac\x82\xc5\x81\xc5\x92\xc5\xa0\xc5\xb8\xc5\xbd\xc4\xb1\xc5\x82\xc5\x93\xc5\xa1\xc5\xbe\xe2\x82\xac\xc2\xa1\xc2\xa2\xc2\xa3\xc2\xa4\xc2\xa5\xc2\xa6\xc2\xa7\xc2\xa8\xc2\xa9\xc2\xaa\xc2\xab\xc2\xac\xc2\xae\xc2\xaf\xc2\xb0\xc2\xb1\xc2\xb2\xc2\xb3\xc2\xb4\xc2\xb5\xc2\xb6\xc2\xb7\xc2\xb8\xc2\xb9\xc2\xba\xc2\xbb\xc2\xbc\xc2\xbd\xc2\xbe\xc2\xbf\xc3\x80\xc3\x81\xc3\x82\xc3\x83\xc3\x84\xc3\x85\xc3\x86\xc3\x87\xc3\x88\xc3\x89\xc3\x8a\xc3\x8b\xc3\x8c\xc3\x8d\xc3\x8e\xc3\x8f\xc3\x90\xc3\x91\xc3\x92\xc3\x93\xc3\x94\xc3\x95\xc3\x96\xc3\x97\xc3\x98\xc3\x99\xc3\x9a\xc3\x9b\xc3\x9c\xc3\x9d\xc3\x9e\xc3\x9f\xc3\xa0\xc3\xa1\xc3\xa2\xc3\xa3\xc3\xa4\xc3\xa5\xc3\xa6\xc3\xa7\xc3\xa8\xc3\xa9\xc3\xaa\xc3\xab\xc3\xac\xc3\xad\xc3\xae\xc3\xaf\xc3\xb0\xc3\xb1\xc3\xb2\xc3\xb3\xc3\xb4\xc3\xb5\xc3\xb6\xc3\xb7\xc3\xb8\xc3\xb9\xc3\xba\xc3\xbb\xc3\xbc\xc3\xbd\xc3\xbe\xc3\xbf",
		  "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7a\x7b\x7c\x7d\x7e\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8a\x8b\x8c\x8d\x8e\x8f\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9b\x9c\x9d\x9e\xa0\xa1\xa2\xa3\xa4\xa5\xa6\xa7\xa8\xa9\xaa\xab\xac\xae\xaf\xb0\xb1\xb2\xb3\xb4\xb5\xb6\xb7\xb8\xb9\xba\xbb\xbc\xbd\xbe\xbf\xc0\xc1\xc2\xc3\xc4\xc5\xc6\xc7\xc8\xc9\xca\xcb\xcc\xcd\xce\xcf\xd0\xd1\xd2\xd3\xd4\xd5\xd6\xd7\xd8\xd9\xda\xdb\xdc\xdd\xde\xdf\xe0\xe1\xe2\xe3\xe4\xe5\xe6\xe7\xe8\xe9\xea\xeb\xec\xed\xee\xef\xf0\xf1\xf2\xf3\xf4\xf5\xf6\xf7\xf8\xf9\xfa\xfb\xfc\xfd\xfe\xff");

  {
    const char *src = "σ";
    fz_try (ctx)
    {
      size_t length;
      pdfout_char_conv (ctx, "UTF-8", "PDFDOC", src, sizeof src - 1, NULL,
			&length);
      test_assert (0);
    }
    fz_catch (ctx)
    {
      test_assert (ctx->error->errcode == FZ_ERROR_ABORT);
    }
  }

  exit (0);
}

static void check_regex (void)
{
  {
    struct pdfout_re_pattern_buffer *
      buff = XZALLOC (struct pdfout_re_pattern_buffer);
    const char *pattern = "ab";
    const char *string = "aaaab";
    int len = strlen (string);
    const char * error_string =
      pdfout_re_compile_pattern (pattern, strlen (pattern),
				 RE_SYNTAX_EGREP, 0, buff);
    if (error_string)
      error (1, errno, "pdfout_re_compile_pattern: %s", error_string);

    test_assert (pdfout_re_search (buff, string, len, 0, len) == 3);
    test_assert (buff->start - string == 3);
    test_assert (buff->end - string == 5);
    
    pdfout_re_free (buff);
  }
  exit (0);
}

static error_t
parse_opt (int key, _GL_UNUSED char *arg, _GL_UNUSED struct argp_state *state)
{
  switch (key)
    {
    case INCREMENTAL_UPDATE: check_incremental_update (); break;
    case INCREMENTAL_UPDATE_XREF: check_incremental_update_xref (); break;
    case STRING_CONVERSIONS: check_string_conversions (); break;
    case REGEX: check_regex (); break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child children[] = {
  {&pdfout_general_argp, 0, NULL, 0},
  {0}
};

static struct argp argp = { options, parse_opt, usage, doc, children };

void
pdfout_command_debug (int argc, char **argv)
{
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
}
