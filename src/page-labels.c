#include "common.h"

/* Update PDF's page labels.  */



static bool
streq (const char *a, const char *b)
{
  return strcmp (a, b) == 0;
}

static bool
strneq (const char *a, const char *b)
{
  return strcmp (a, b) != 0;
}

static void
assert_c_string (fz_context *ctx, const char *s, int len)
{
  if (strlen(s) != len)
    pdfout_throw (ctx, "unexpected string with embedded null bytes");
}

static char *
scalar_to_string (fz_context *ctx, pdfout_data *scalar)
{
  int value_len;
  char *value = pdfout_data_scalar_get (ctx, scalar, &value_len);
  assert_c_string (ctx, value, value_len);
  return value;
}
static int
scalar_to_int (fz_context *ctx, pdfout_data *scalar)
{
  char *string = scalar_to_string (ctx, scalar);
  return pdfout_strtoint_null (ctx, string);
}

/* Return page number.  */
static int
check_hash (fz_context *ctx, pdfout_data *hash, int previous_page)
{

  pdfout_data *scalar = pdfout_data_hash_gets (ctx, hash, "page");
  if (scalar == NULL)
    pdfout_throw (ctx, "missing 'page' in pagelabels hash");

  int page = scalar_to_int (ctx, scalar);

  scalar = pdfout_data_hash_gets (ctx, hash, "first");
  if (scalar)
    {
      int first = scalar_to_int (ctx, scalar);
      if (first < 1)
	    pdfout_throw (ctx, "value of key 'first' must be >= 1");
    }

  scalar = pdfout_data_hash_gets (ctx, hash, "style");
  if (scalar)
    {
      char *style = scalar_to_string (ctx, scalar);
       if (strneq (style, "arabic") && strneq (style, "Roman")
	      && strneq (style, "roman") && strneq (style, "Letters")
	      && strneq (style, "letters"))
	    pdfout_throw (ctx, "invalid style '%s'", style);
    }
  
  if (previous_page == 0 && page != 1)
    pdfout_throw (ctx, "first page must be 1");
  if (page <= previous_page)
    pdfout_throw (ctx, "page numbers must increase");
  
  return page;
}

static void
check_page_labels (fz_context *ctx, pdfout_data *labels)
{
  int len = pdfout_data_array_len (ctx, labels);
  if (len < 1)
    pdfout_throw (ctx, "empty page labels");
  
  int previous_page = 0;
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, labels, i);
      previous_page = check_hash (ctx, hash, previous_page);
    }
  
}

static pdf_obj *
hash_to_pdf_dict (fz_context *ctx, pdf_document *doc, pdfout_data *hash,
		  int *page)
{
  pdf_obj *dict_obj = pdf_new_dict (ctx, doc, 3);
  int len = pdfout_data_hash_len (ctx, hash);
  for (int j = 0; j < len; ++j)
    {
      char *key, *value;
      int value_len;
      pdfout_data_hash_get_key_value (ctx, hash, &key, &value, &value_len, j);

      if (streq (key, "page"))
	*page = pdfout_strtoint_null (ctx, value);
      else if (streq (key, "prefix"))
	{
	  pdf_obj *string = pdfout_utf8_to_str_obj (ctx, doc, value,
						    value_len);
	  pdf_dict_puts_drop (ctx, dict_obj, "P", string);
	}
      else if (streq (key, "first"))
	{
	  int first = pdfout_strtoint_null (ctx, value);
	  pdf_dict_puts_drop (ctx, dict_obj, "St",
			      pdf_new_int (ctx, doc, first));
	}
      else if (streq (key, "style"))
	{
	  const char *name;
	  if (streq (value, "arabic"))
	    name = "D";
	  else if (streq (value, "Roman"))
	    name = "R";
	  else if (streq (value, "roman"))
	    name = "r";
	  else if (streq (value, "Letters"))
	    name = "A";
	  else if (streq (value, "letters"))
	    name = "a";
	  else
	    abort ();
	  pdf_obj *name_obj = pdf_new_name (ctx, doc, name);
	  pdf_dict_puts_drop (ctx, dict_obj, "S", name_obj);
	}
     
    }

  return dict_obj;
}

void
pdfout_page_labels_set (fz_context *ctx, pdf_document *doc,
			pdfout_data *labels)
{
  if (labels)
    check_page_labels (ctx, labels);
  
  pdf_obj *root = pdf_dict_get (ctx, pdf_trailer (ctx, doc), PDF_NAME_Root);
  
  if (root == NULL)
    pdfout_throw (ctx, "no document catalog, cannot set/unset page labels");
  
  if (labels == NULL)
    {
      /* Remove page labels.  */
      pdf_dict_dels (ctx, root, "PageLabels");
      return;
    }

  int num = pdfout_data_array_len (ctx, labels);
  pdf_obj *array_obj = pdf_new_array (ctx, doc, 2 * num);
  
  for (int i = 0; i < num; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, labels, i);
      int page;
      pdf_obj *dict_obj = hash_to_pdf_dict (ctx, doc, hash, &page);

      pdf_obj *page_obj = pdf_new_int (ctx, doc, page);
      pdf_array_push_drop (ctx, array_obj, page_obj);
      
      pdf_array_push_drop (ctx, array_obj, dict_obj);
    }
      
  pdf_obj *labels_obj = pdf_new_dict (ctx, doc, 1);
  pdf_dict_puts_drop (ctx, labels_obj, "Nums", array_obj);
  pdf_dict_puts_drop (ctx, root, "PageLabels", labels_obj);
  
}

static void
push_int_key (fz_context *ctx, pdfout_data *hash, const char *key, int num)
{
  char buf[200];
  pdfout_snprintf(ctx, buf, "%d", num);
  int buf_len = strlen(buf);
  pdfout_data_hash_push_key_value (ctx, hash, key, buf, buf_len);
}

static void
parse_dict (fz_context *ctx, pdf_obj *dict, pdfout_data *hash)
{
  /* Check Style key. */
  
  pdf_obj *style = pdf_dict_get (ctx, dict, PDF_NAME_S);
  if (style)
    {
      const char *value;
      if (pdf_is_name (ctx, style) == false)
	pdfout_throw (ctx, "style key 'S' not a name object");
      /* pdf_to_name returns the empty string, not NULL, on all errors,
	 so the strcmps are allowed.  */
      if (pdf_name_eq (ctx, style, PDF_NAME_D))
	value = "arabic";
      else if (pdf_name_eq (ctx, style, PDF_NAME_R))
	value = "Roman";
      else if (pdf_name_eq (ctx, style, PDF_NAME_A))
	value = "Letters";
      else
	{
	  /* FIXME once PDF_NAMES for "r" and "a" available.  */
	  const char *string = pdf_to_name (ctx, style);
	  if (streq(string, "r"))
	    value = "roman";
	  else if (streq (string, "a"))
	    value = "letters";
	  else
	    pdfout_throw (ctx, "unknown numbering style '%s'", string);
	}

      int len = strlen (value);
      pdfout_data_hash_push_key_value (ctx, hash, "style", value, len);
    }
  
  pdf_obj *first = pdf_dict_gets (ctx, dict, "St");
  if (first)
    {
      int value  = pdf_to_int (ctx, first);
      if (value < 1)
	pdfout_throw (ctx, "value %d of 'St' is < 1 or not an int", value);

      push_int_key (ctx, hash, "first", value);
    }
      
  pdf_obj *prefix = pdf_dict_get (ctx, dict, PDF_NAME_P);
  if (prefix)
    {
      if (pdf_is_string (ctx, prefix) == false)
	pdfout_throw (ctx, "value of 'P' not a string");

      int utf8_len;
      char *utf8 = pdfout_str_obj_to_utf8 (ctx, prefix, &utf8_len);
      pdfout_data_hash_push_key_value (ctx, hash, "prefix", utf8, utf8_len);
      free (utf8);
    }
}  
      
pdfout_data *
pdfout_page_labels_get (fz_context *ctx, pdf_document *doc)
{
  pdf_obj *trailer = pdf_trailer (ctx, doc);
  pdf_obj *labels_obj = pdf_dict_getp (ctx, trailer, "Root/PageLabels");
  pdf_obj *array = pdf_dict_gets (ctx, labels_obj, "Nums");

  if (array && pdf_is_array (ctx, array) == false)
    pdfout_throw (ctx, "Nums is not an array");
		  
  int length = pdf_array_len (ctx, array);

  pdfout_data *labels = pdfout_data_array_new (ctx);

  for (int i = 0; i < length / 2; ++i)
    {
      pdf_obj *object = pdf_array_get (ctx, array, 2 * i);
      
      if (pdf_is_int (ctx, object) == false)
	pdfout_throw (ctx, "key in number tree not an int");

      pdfout_data *hash = pdfout_data_hash_new (ctx);
      
      int page = pdf_to_int (ctx, object);
      if (page < 0)
	pdfout_throw (ctx, "key in number tree is < 0");

      push_int_key (ctx, hash, "page", page);
      
      pdf_obj *dict = pdf_array_get (ctx, array, 2 * i + 1);
      if (pdf_is_dict (ctx, dict) == false)
	pdfout_throw (ctx, "value in number tree not a dict");

      parse_dict (ctx, dict, hash);
      
      pdfout_data_array_push (ctx, labels, hash);
    }
  
  return labels;
}
