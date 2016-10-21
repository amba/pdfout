#include "common.h"
#include "shared.h"
#include "charset-conversion.h"
#include "data.h"

static fz_context *ctx;

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

#define assert_throw(ctx, expr)					\
  do								\
    {								\
      bool failed = false;					\
      fz_try (ctx) { expr; } fz_catch (ctx) {failed = true;}		\
      if (failed == false)						\
	{								\
	  fprintf (stderr, "at file: %s, line: %d\n\
Expression '" #expr "' did not throw as expected\n", __FILE__, __LINE__); \
	  exit (1);							\
	}								\
    } while (0)


static void
write_pdf (pdf_document *doc, pdfout_tmp_stream *handle,
	   pdf_write_options *opts)
{
  fz_output *output = pdfout_tmp_stream_output (ctx, handle);
  pdf_write_document (ctx, doc, output, opts);
}

static pdf_document *
open_pdf (pdfout_tmp_stream *handle)
{
  fz_stream *stream = pdfout_tmp_stream_stream (ctx, handle);
  pdf_document *doc = pdf_open_document_with_stream (ctx, stream);
  if (doc->repair_attempted || doc->freeze_updates)
    error (1, 0, "pdf_open_document: broken_pdf");
  return doc;
}

static void
check_incremental_update (void)
{
  pdf_write_options opts = { 0 };
  pdf_obj *info;
  const char *subject = "check incremental update";
  int num;

  pdf_document *doc = pdf_create_document (ctx);

  pdfout_tmp_stream *handle = pdfout_tmp_stream_new (ctx);

  write_pdf (doc, handle, &opts);
  pdf_drop_document (ctx, doc);
  doc = open_pdf (handle);

  num = pdf_create_object (ctx, doc);
  info = pdf_new_dict (ctx, doc, 1);
  pdf_update_object (ctx, doc, num, info);
  pdf_drop_obj (ctx, info);

  pdf_dict_puts_drop (ctx, info, "Subject",
		      pdf_new_string (ctx, doc, subject, strlen (subject)));
  pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc), "Info",
		      pdf_new_indirect (ctx, doc, num, 0));

  opts.do_incremental = 1;
  
  write_pdf (doc, handle, &opts);
  pdf_drop_document (ctx, doc);
  doc = open_pdf (handle);
  
  
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
  /* to run Memcheck-clean, keep a pointer to each doc  */
  pdf_document *doc[3];
  pdf_obj *info;
  const char *subject = "check incremental update with xref";
  int num;

  doc[0] = pdf_create_document (ctx);

  opts.do_incremental = 0;

  pdfout_tmp_stream *handle = pdfout_tmp_stream_new (ctx);
  write_pdf (doc[0], handle, &opts);
  doc[1] = open_pdf (handle);
  
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
  write_pdf (doc[1], handle, &opts);
  doc[2] = open_pdf (handle);
  
  exit (0);
}

static void print_string (const char *string, size_t len)
{
  fprintf (stderr, "length: %zu, string:\n", len);
  for (size_t i = 0; i < len; ++i)
    fprintf (stderr, " %02x", (unsigned char) string[i]);
  fprintf (stderr, "\n");
}



/* Check that from -> too -> from is the identity.  */
#define test_conversion(from, too, src, expected)			\
  do									\
    {									\
      int buffer_len, buffer_len_back;				\
      char *result, *result_back;					\
      result = pdfout_char_conv (ctx, from, too, src, sizeof src - 1,	\
				 &buffer_len);				\
      test_equal (result, expected, buffer_len, sizeof expected - 1);	\
      result_back = pdfout_char_conv (ctx, too, from, result, buffer_len, \
				      &buffer_len_back);		\
      test_equal (result_back, src, buffer_len_back, sizeof src - 1);	\
      free (result);							\
      free (result_back);						\
    } while (0);

#define test_from_utf8(too, src, expected)	\
  test_conversion ("UTF-8", too, src, expected)

#define test_pdf_from_utf8(src, expected)				\
  do									\
    {									\
      int buffer_len, buffer_len_back;				\
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
      int length;
      pdfout_char_conv (ctx, "UTF-8", "PDFDOC", src, sizeof src - 1,
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

static void check_json_parser_value (fz_context *ctx, const char *json,
				     const char *value, bool fail)
{
  fz_stream *stm = fz_open_memory (ctx, (unsigned char *) json,
				   strlen (json));
      
  pdfout_parser *parser = pdfout_parser_json_new (ctx, stm);

  if (fail == true)
    {
      assert_throw (ctx, pdfout_parser_parse (ctx, parser));
    }
  else
    {
      pdfout_data *data;
      fz_try (ctx)
      {
	data = pdfout_parser_parse (ctx, parser);
      }
      fz_catch (ctx)
      {
	fprintf (stderr, "Unexpected throw for json string '%s'\n", json);
	exit (1);
      }
  
      test_assert (pdfout_data_is_scalar (ctx, data));
      int len;
      char *result = pdfout_data_scalar_get (ctx, data, &len);
      test_equal (result, value, len, strlen (value));
      pdfout_data_drop (ctx, data);
    }
  
  pdfout_parser_drop (ctx, parser);
  fz_drop_stream (ctx, stm);
      
}

static void check_json_parser_values (fz_context *ctx)
{
  {
    struct test {
      const char *text;
      bool fail;
    };
    
    struct test tests[] = {
      {"1"},
      {"0"},
      {"-1"},
      {"1.1"},
      {"0.1"},
      {"1e+2"},
      {"1e-0"},
      {"1.1e2"},
      {"a", true},
      {"01", true},
      {"000", true},
      {".1", true},
      {"12.", true},
      {"1e+ ", true},
      {".1e1", true},
      {0}
    };
    for (int i = 0; tests[i].text; ++i)
      check_json_parser_value (ctx, tests[i].text, tests[i].text,
			       tests[i].fail);
  }
  
  {
    /* Check reallocation in the value buffer.  */
    char big[1000];
    char big_expected[1000];
    big[0] = '"';
    memset (big + 1, 'x', 997);
    big[998] = '"';
    big[999] = 0;
    memset (big_expected, 'x', 997);
    big_expected[997] = 0;
    
    struct test {
      const char *text;
      const char *value;
      bool fail;
    } tests[] = {
      {"\"\"", ""},
      {"\"x\"", "x"},
      {"\"ℕ\"", "ℕ"},
      {"\"abc\\ndef\"", "abc\ndef"},
      {"\"\\u000a\"", "\n"},
      {"\"\\uD834\\udd1e\\uD834\\udd1eabc\"", "𝄞𝄞abc"},
      {"\"\\uD853\\uDF5C\"", "\U00024F5C"},
      {big, big_expected},
      {"\"x", 0, true},
      {"\"x\x01\"", 0, true},
      {"\"x\n\"", 0, true},
      {"\"\\x\"", 0, true},
      /* Single leading surrogate.  */
      {"\"\\uD800\"", 0, true},
      /* Leading surrogate not followed by trailing surrogate.  */
      {"\"\\uD800\\u000a\"", 0, true},
      {"\"\\uD800\\uD800\"", 0, true},
      /* Broken UTF-8.  */
      {"\"abcd\xfe\xff\"", 0, true},
      {0}
    };
    for (int i = 0; tests[i].text; ++i)
      check_json_parser_value (ctx, tests[i].text, tests[i].value,
			       tests[i].fail);
  }
}

static void
json_parser_test (fz_context *ctx, const char *json, pdfout_data *result)
{
  fprintf (stderr, "testing json '%s'\n", json);


  fz_stream *stm = fz_open_memory (ctx, (unsigned char *) json,
				   strlen (json));
      
  pdfout_parser *parser = pdfout_parser_json_new (ctx, stm);

  if (result == NULL)
      assert_throw (ctx, pdfout_parser_parse (ctx, parser));
  else
    {
      pdfout_data *data = pdfout_parser_parse (ctx, parser);

      test_assert (pdfout_data_cmp (ctx, result, data) == 0);
  
      pdfout_data_drop (ctx, result);
      pdfout_data_drop (ctx, data);
    }
  pdfout_parser_drop (ctx, parser);
  fz_drop_stream (ctx, stm);
}

static void array_push_string (fz_context *ctx, pdfout_data *array,
			       const char *string)
{
  pdfout_data *s = pdfout_data_scalar_new (ctx, string, strlen (string));
  pdfout_data_array_push (ctx, array, s);
}

static void hash_push_string (fz_context *ctx, pdfout_data *hash,
			      const char *key, const char *value)
{
  pdfout_data *k = pdfout_data_scalar_new (ctx, key, strlen (key));
  pdfout_data *v = pdfout_data_scalar_new (ctx, value, strlen (value));
  pdfout_data_hash_push (ctx, hash, k, v);
}


static void check_json_parser (fz_context *ctx)
{
  struct test {
    const char *json;
    pdfout_data *result;
  };

  struct test tests[] = {
    {""}, {"x"}, {"1 1"}, {"["},{"]"}, {"{"}, {"}"}, {"[1, 2"}, {"[1 1]"},
    {"[1,]"}, {"[1, 2 3]"}, {"[1]]"}, {"{false: 1}"}, {"{"}, {"}"}, {0}};
  
  for (int i = 0; tests[i].json; ++i)
    json_parser_test (ctx, tests[i].json, tests[i].result);

  const char *json = "[1, null, true, [], {}, false, {\"abc\": 1,\
 \"def\" : true}]";

  pdfout_data *a = pdfout_data_array_new (ctx);
  array_push_string (ctx, a, "1");
  array_push_string (ctx, a, "null");
  array_push_string (ctx, a, "true");
  pdfout_data_array_push (ctx, a, pdfout_data_array_new (ctx));
  pdfout_data_array_push (ctx, a, pdfout_data_hash_new (ctx));
  array_push_string (ctx, a, "false");

  pdfout_data *h = pdfout_data_hash_new (ctx);
  pdfout_data_array_push (ctx, a, h);
  hash_push_string (ctx, h, "abc", "1");
  hash_push_string (ctx, h, "def", "true");

  json_parser_test (ctx, json, a);
}

static void json_emitter_test (fz_context *ctx, pdfout_data *data,
			       const char *expected)
{
  fz_buffer *out_buf = fz_new_buffer (ctx, 0);
  fz_output *out = fz_new_output_with_buffer (ctx, out_buf);
  pdfout_emitter *emitter = pdfout_emitter_json_new (ctx, out);
  pdfout_emitter_emit (ctx, emitter, data);

  if (strlen (expected) != out_buf->len
      || memcmp (expected, out_buf->data, out_buf->len))
    {
      fprintf (stderr, "json_emitter_test: expected:\n%s"
	       "got:\n%.*s\n", expected, (int) out_buf->len,
	       (char *) out_buf->data);
      abort ();
    }
      
  pdfout_emitter_drop (ctx, emitter);
  fz_drop_output (ctx, out);
  fz_drop_buffer (ctx, out_buf);
  pdfout_data_drop (ctx, data);
}

static void check_json_emitter (fz_context *ctx)
{
  {
    char json[1000];
    int len = 0;

    /* test escapes */
    json[len++] = 0;
    json[len++] = 1;
    json[len++] = '"';
    json[len++] = '\\';
    json[len++] = '/';
    json[len++] = '\b';
    json[len++] = '\f';
    json[len++] = '\n';
    json[len++] = '\r';
    json[len++] = '\t';
    
    const char *expected = "\"" "\\u0000" "\\u0001" "\\\"" "\\\\" "/" "\\b"
      "\\f" "\\n" "\\r" "\\t" "\"" "\n";
    
    pdfout_data *data = pdfout_data_scalar_new (ctx, json, len);
    json_emitter_test (ctx, data, expected);
  }
  { 
    pdfout_data *a = pdfout_data_array_new (ctx);
    array_push_string (ctx, a, "1");
    array_push_string (ctx, a, "null");
    array_push_string (ctx, a, "true");
    pdfout_data_array_push (ctx, a, pdfout_data_array_new (ctx));
    pdfout_data_array_push (ctx, a, pdfout_data_hash_new (ctx));
    array_push_string (ctx, a, "false");

    pdfout_data *h = pdfout_data_hash_new (ctx);
    pdfout_data_array_push (ctx, a, h);
    hash_push_string (ctx, h, "abc", "1");
    hash_push_string (ctx, h, "def", "true");

    const char *expected = "\
[\n\
  1,\n\
  null,\n\
  true,\n\
  [],\n\
  {},\n\
  false,\n\
  {\n\
    \"abc\": 1,\n\
    \"def\": true\n\
  }\n\
]\n";

    json_emitter_test (ctx, a, expected);
  }


}
static void check_json (void)
{
  check_json_parser_values (ctx);

  check_json_parser (ctx);

  check_json_emitter (ctx);
  exit (0);
}

static void check_data (void)
{
  pdfout_data *hash = pdfout_data_hash_new (ctx);

  const char *s1 = "key1";
  const char *s2 = "value1";
  const char *s3 = "key2";
  pdfout_data *key1 = pdfout_data_scalar_new (ctx, s1, strlen (s1));
  pdfout_data *value1 = pdfout_data_scalar_new (ctx, s2, strlen (s2));

  pdfout_data_hash_push (ctx, hash, key1, value1);

  test_assert (pdfout_data_hash_len (ctx, hash) == 1);

  pdfout_data *key2 = pdfout_data_scalar_new (ctx, s3, strlen (s3));
  pdfout_data *array = pdfout_data_array_new (ctx);

  pdfout_data_hash_push (ctx, hash, key2, array);

  for (int i = 0; i < 100; ++i)
    {
      pdfout_data *item = pdfout_data_scalar_new (ctx, s1, strlen (s1));
      pdfout_data_array_push (ctx, array, item);
      test_assert (pdfout_data_array_len (ctx, array) == i + 1);
    }

  test_assert (pdfout_data_array_len (ctx, array) == 100);
  
  test_assert (pdfout_data_hash_len (ctx, hash) == 2);
  
  pdfout_data_drop (ctx, hash);
  exit (0);
}
enum {
  INCREMENTAL_UPDATE = CHAR_MAX + 1,
  INCREMENTAL_UPDATE_XREF,
  STRING_CONVERSIONS,
  JSON,
  DATA,
};

static struct option longopts[] = {
  {"incremental-update", no_argument, NULL, INCREMENTAL_UPDATE},
  {"incremental-update-xref", no_argument, NULL, INCREMENTAL_UPDATE_XREF},
  {"string-conversions", no_argument, NULL, STRING_CONVERSIONS},
  {"json", no_argument, NULL, JSON},
  {"data", no_argument, NULL, DATA},
  {NULL, 0 , NULL, 0}
};

static void
print_usage (void)
{
  printf ("Usage: %s COMMAND_OPTION\n", pdfout_program_name);
}

static void
print_help (void)
{
  print_usage ();
  puts("\
Run whitebox test.\n\
\n\
 Tests:\n\\n\
      --incremental-update\n\
      --incremental-update-xref\n\
      --string-conversions\n\
      --json\n\
      --data\n\
\n\
 general options:\n\
  -h, --help                 Give this help list\n\
  -u, --usage                Give a short usage message\n\
");
}

static void
parse_options (int argc, char **argv)
{
  int optc;
  while ((optc = getopt_long (argc, argv, "hu", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'h': print_help (); exit (0);
	case 'u': print_usage (); exit (0);
	case INCREMENTAL_UPDATE: check_incremental_update (); break;
	case INCREMENTAL_UPDATE_XREF: check_incremental_update_xref (); break;
	case STRING_CONVERSIONS: check_string_conversions (); break;
	case JSON: check_json (); break;
	case DATA: check_data (); break;
	default:
	  print_usage ();
	  exit (1);
	}
    }
  abort();
}

void
pdfout_command_debug (fz_context *ctx_arg, int argc, char **argv)
{
  ctx = ctx_arg;
  parse_options (argc, argv);
}
