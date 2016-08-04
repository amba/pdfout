#include "common.h"
#include "page-labels.h"
#include "charset-conversion.h"
#include <argmatch.h>

/* Update PDF's page labels.  */

static void
assert_c_string(fz_context *ctx, const char *s, int len)
{
  if (strlen(s) != len)
    pdfout_throw (ctx, "unexpected string with embedded null bytes");
}

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

/* Return page number.  */
static int
check_hash (fz_context *ctx, pdfout_data *hash, int previous_page)
{
  int len = pdfout_data_hash_len (ctx, hash);
  int page;
  for (int i = 0; i < len; ++i)
    {
      char *key, *value;
      int value_len;
      
      pdfout_data_hash_get_key_value (ctx, hash, &key, &value, &value_len, i);
      
      if (streq (key, "page"))
	{
	  assert_c_string (value, value_len);
	  page = pdfout_strtoint_null (ctx, value);
	}
      else if (streq (key, "first"))
	{
	  assert_c_string (value, value_len);
	  int first = pdfout_strtoint_null (ctx, value);
	  if (first < 1)
	    pdfout_throw (ctx, "value of key 'first' must be >= 1");
	}
      else if (streq (key, "style"))
	{
	  assert_c_string (value, value_len);
	  if (strneq (value, "arabic") && strneq (value, "Roman")
	      && strneq (value, "roman") && strneq (value, "Letters")
	      && strneq (value, "letters"))
	    pdfout_throw (ctx, "invalid style '%s'", value);
	}
      /* prefix is arbitrary -> no check. */
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
utf8_to_pdf_string (fz_context *ctx, pdf_document *doc, const char *utf8,
		    int utf8_len)
{
  int text_len;
  char *text = pdfout_utf8_to_pdf (ctx, utf8, utf8_len, &text_len);

  /* FIXME: do proper try/catch for the allocations. */
  pdf_obj *string_obj = pdf_new_string (ctx, doc, text, text_len);

  free (text);

  return string_obj;
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
      pdfout_data_hash_get_key_value (ctx, hash, &key, &value, &value_len);

      if (streq (key, "page"))
	*page = pdfout_strtoint_null (ctx, value);
      else if (streq (key, "prefix"))
	{
	  pdf_obj *string = utf8_to_pdf_string (ctx, doc, value, value_len);
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
	  char *name;
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
  
  for (i = 0; i < num; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, labels, i);
      int page;
      pdf_obj *dict_obj = hash_to_pdf_dict (ctx, doc, hash);

      pdf_obj *page_obj = pdf_new_int (ctx, doc, page);
      pdf_array_push_drop (ctx, array_obj, page_obj);
      
      pdf_array_push_drop (ctx, array_obj, dict_obj);
    }
      
  pdfo_obj *labels_obj = pdf_new_dict (ctx, doc, 1);
  pdf_dict_puts_drop (ctx, labels_obj, "Nums", array_obj);
  pdf_dict_puts_drop (ctx, root, "PageLabels", labels_obj);
  
}

static int
page_labels_get (pdfout_page_labels_t *labels, fz_context *ctx,
		 pdf_document *doc)
{
  bool broken = false;
  pdf_obj *array, *labels_obj;
  int length, i;
  
  labels_obj = pdf_dict_getp (ctx, pdf_trailer (ctx, doc), "Root/PageLabels");
  
  if (labels_obj == NULL)
    MSG_RETURN (0, "no 'PageLabels' entry in document catalogue");
  
  array = pdf_dict_gets (ctx, labels_obj, "Nums");
  if (pdf_is_array (ctx, array) == false)
    MSG_RETURN (1, "key 'Nums' not an array");
  
  length = pdf_array_len (ctx, array);
  if (length == 0)
    MSG_RETURN (1, "empty 'PageLabels' entry");
  
  if (length % 2)
    {
      MSG ("uneven number in Nums array");
      broken = true;
    }
  
  for (i = 0; i < length / 2; ++i)
    {
      pdfout_page_labels_mapping_t mapping = {0};
      pdf_obj *dict, *object;

      object = pdf_array_get (ctx, array, 2 * i);
      
      if (pdf_is_int (ctx, object) == false)
	{
	  MSG ("key in number tree not an int");
	  broken = true;
	  continue;
	}
      
      mapping.page = pdf_to_int (ctx, object);
      
      if (mapping.page < 0)
	{
	  MSG ("key in number tree is < 0");
	  broken = true;
	  continue;
	}
      
      dict = pdf_array_get (ctx, array, 2 * i + 1);
      if (pdf_is_dict (ctx, dict) == false)
	{
	  MSG ("value in number tree not a dict");
	  broken = true;
	  continue;
	}
      
      object = pdf_dict_get (ctx, dict, PDF_NAME_S);
      if (object)
	{
	  if (pdf_is_name (ctx, object) == false)
	    {
	      MSG ("'S' not a name object");
	      broken = true;
	    }
	  else
	    {
	      /* pdf_to_name returns the empty string, not NULL, on all errors,
		 so the strcmps are allowed.  */
	      if (pdf_name_eq (ctx, object, PDF_NAME_D))
		mapping.style = PDFOUT_PAGE_LABELS_ARABIC;
	      else if (pdf_name_eq (ctx, object, PDF_NAME_R))
		mapping.style = PDFOUT_PAGE_LABELS_UPPER_ROMAN;
	      else if (pdf_name_eq (ctx, object, PDF_NAME_A))
		mapping.style = PDFOUT_PAGE_LABELS_UPPER;
	      else
		{
		  /* FIXME once PDF_NAMES for "r" and "a" available.  */
		  const char *style = pdf_to_name (ctx, object);
		  if (STREQ (style, "r"))
		    mapping.style = PDFOUT_PAGE_LABELS_LOWER_ROMAN;
		  else if (STREQ (style, "a"))
		    mapping.style = PDFOUT_PAGE_LABELS_LOWER;
		  else
		    {
		      MSG ("unknown numbering style '%s'", style);
		      broken = true;
		    }
		}
	    }
	}
      object = pdf_dict_gets (ctx, dict, "St");
      if (object)
	{
	  if (pdf_is_int (ctx, object) == false)
	    {
	      MSG ("'St' not an int");
	      broken = true;
	    }
	  mapping.start = pdf_to_int (ctx, object);
	  if (mapping.start < 1)
	    {
	      MSG ("value %d of 'St' is < 1", mapping.start);
	      broken = true;
	      mapping.start = 0;
	    }
	}
      
      object = pdf_dict_get (ctx, dict, PDF_NAME_P);
      if (object)
	{
	  if (pdf_is_string (ctx, object) == false)
	    {
	      MSG ("'P' not a string");
	      broken = true;
	    }
	  else
	    {
	      const char *pdf_buf = pdf_to_str_buf (ctx, object);
	      int pdf_buf_len = pdf_to_str_len (ctx, object);
	      if (pdf_buf_len)
		{
		  int utf8_buf_len;
		  mapping.prefix = pdfout_pdf_to_utf8 (ctx, pdf_buf,
						       pdf_buf_len,
						       &utf8_buf_len);
		}
	    }
	}
      
      pdfout_page_labels_push (labels, &mapping);
      free (mapping.prefix);
    }
  
  return broken;
}

pdfout_data *
pdfout_page_labels_get (fz_context *ctx, pdf_document *doc)
{
  *labels = pdfout_page_labels_new ();
  rv = page_labels_get (*labels, ctx, doc);
  if (pdfout_page_labels_length (*labels) == 0)
    {
      pdfout_page_labels_free (*labels);
      *labels = NULL;
    }
  return rv;
}
