
#include "common.h"
#include "outline-wysiwyg.h"

static bool
streq (const char *a, const char *b)
{
  return strcmp (a, b) == 0;
}

static const char *
data_hash_get_string_key (fz_context *ctx, pdfout_data *hash,
			  const char *key)
{
  int len;
  const char *value = pdfout_data_hash_get_key_value (ctx, hash, key, &len);
  if (strlen (value) != len)
    pdfout_throw (ctx, "embedded null byte for key '%s'", key);
  return value;
}

static const char *
data_array_get_string (fz_context *ctx, pdfout_data *array, int i)
{
  pdfout_data *scalar = pdfout_data_array_get (ctx, array, i);
  int len;
  const char *result = pdfout_data_scalar_get (ctx, array, &len);
  if (len != strlen (result))
    pdfout_throw (ctx, "string with embedded null byte");
  return result;
}

static int
dest_sequence_length (fz_context *ctx, const char *label)
{
  if (streq (label, "XYZ"))
    return 4;
  else if (streq (label, "Fit"))
    return 1;
  else if (streq (label, "FitH") || streq (label, "FitV"))
    return 2;
  else if (streq (label, "FitR"))
    return 5;
  else
    pdfout_throw (ctx, "unknown destination: '%s', label");
}

static void
check_sequence_numbers (fz_context *ctx, pdfout_data *dest)
{
  int len = pdfout_data_array_len (ctx, dest);
  for (int i = 1; i < len; ++i)
    {
      char *num = data_array_get_string (ctx, dest, i);
      if (streq (num, "null"))
	continue;
      else if (isnan (pdfout_strtof_nan (num)))
	pdfout_throw (ctx, "not a float: '%s'", num);
    }
}
			
static void
check_dest_sequence (fz_context *ctx, pdfout_data *dest)
{
  int len = pdfout_data_array_len (ctx, dest);
  if (len < 1 || length > 5)
    pdfout_throw (ctx, "illegal View sequence with length %d", len);

  const char *label = data_array_get_string (ctx, dest, 0);
  
  if (len != dest_sequence_length (ctx, label))
    pdfout_data_throw (ctx, "illegal %s sequence with length %d", label, len);

  check_sequence_numbers (ctx, dest);
}

static void
check_outline_hash (fz_context *ctx, pdf_document *doc, pdfout_data *hash)
{
  int len = pdfout_data_hash_len (ctx, hash);
  bool has_title = false, has_page = false;
  
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *key = pdfout_data_hash_get_key (ctx, hash, i);
      pdfout_data *value = pdfout_data_hash_get_value (ctx, hash, i);
      if (pdfout_data_scalar_eq (key, "title"))
	{
	  pdfout_data_scalar_get (ctx, value);
	  has_page = true;
	}
      else if (pdfout_data_scalar_eq (key, "page"))
	{
	  char *s = pdfout_data_scalar_get (ctx, value);
	  int page = pdfout_strtoint_null (ctx, s);
	  int count = pdf_count_pages (ctx, doc);
	  if (page < 1)
	    pdfout_throw (ctx, "page number '%d' is not positive", page);
	  if (page > count)
	    pdfout_throw (ctx,
			  "page number '%d' is bigger than page count %d",
			  page, count);

	  has_page = true;
	}
      else if (pdfout_data_scalar_eq (key, "view"))
	check_dest_sequence (ctx, value);
      else if (pdfout_data_scalar_eq (key, "open"))
	{
	  if (pdfout_data_scalar_eq (value, "true") == false
	      && pdfout_data_scalar_eq (value, "false") == false)
	    pdfout_throw (ctx, "value of key 'open' not a bool");
	}
      else if (pdfout_data_scalar_eq (key, "kids"))
	check_outline_array (ctx, doc, value);
    }

  if (has_title == false)
    pdfout_throw (ctx, "missing title in outline hash");
  if (has_page == false)
    pdfout_throw (ctx, "missing page in outline hash");
  
}

static void
check_outline_array (fz_context *ctx, pdf_document *doc, pdfout_data *outline)
{
  int len = pdfout_data_array_len (ctx, outline);
  if (len < 1)
    pdfout_throw (ctx, "empty outline array");
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, outline, i);
      check_outline_hash (ctx, doc, hash);
    }
}

static void
check_outline (fz_context *ctx, pdf_document *doc, pdfout_data *outline)
{
  int len = pdfout_data_array_len (ctx, outline);

  if (len == 0)
    return;
  
  check_outline_array (ctx, doc, outline);
}

static int
calculate_kids_count (fz_context *ctx, pdfout_data *hash)
{
  pdfout_data *kids = pdfout_data_hash_gets (ctx, hash, "kids");
  if (kids == NULL)
    return 0;
  int kids_len = pdfout_data_array_len (ctx, kids);
  int count = kids_len;
  for (int i = 0; i < kids_len; ++i)
    {
      pdfout_data *kid_hash = pdfout_data_array_get (ctx, kids, i);
      int kid_count = calculate_kids_count (doc, kid_hash);
      if (kid_count > 0)
	count += kid_count;
    }

  const char *open = data_hash_get_string_key (ctx, hash, "open");
  if (streq (open, "false"))
    count *= -1;
  char buf[200];
  pdfout_snprintf (ctx, buf, "%d", count);
  pdfout_data *key = pdfout_data_scalar_new (ctx, "open", strlen (open));
  pdfout_data *value = pdfout_data_scalar_new (ctx, buf, strlen (buf));
  pdfout_data_hash_push (ctx, hash, key, value);

  return count;
}

static void
calculate_counts (fz_context *ctx, pdfout_data *outline)
{
  int len = pdfout_data_array_len (ctx, outline);
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, outline, i);
      calculate_kids_counts (ctx, hash);
    }
}

static pdf_obj *
copy_indirect_ref (fz_context *ctx, pdf_document *doc, pdf_obj *ref)
{
  int num = pdf_to_num (ctx, ref);
  int gen = pdf_to_gen (ctx, ref);
  return pdf_new_indirect (ctx, doc, num, gen);
}

static pdf_obj *
default_view_array (fz_context *ctx, pdf_document *doc, pdf_obj *dest_array)
{
  pdf_array_push (ctx, dest_array, pdf_new_name (ctx, doc, "XYZ"));
  for (int i = 0; i < 3; ++i)
    pdf_array_push (ctx, dest_array, pdf_new_null (ctx, doc));

  return dest_array;
}

static pdf_obj *
convert_dest_array (fz_context *ctx, pdf_document *doc, pdfout_data *view,
		    int page)
{
  pdf_obj *dest_array = pdf_new_array (ctx, doc, 5);
  pdf_obj *page_ref = pdf_lookup_page_obj (ctx, doc, page - 1);
  pdf_array_push (ctx, dest_array, page_ref);
  if (view == NULL)
    return default_view_array (ctx, doc, dest_array);
  int len = pdfout_data_array_len (ctx, view);
  for (int i = 0; i < len; ++i)
    {
      pdf_obj *null_or_real;
      pdfout_data *scalar = pdfout_data_array_get (ctx, view, i);
      if (pdfout_data_scalar_eq (scalar, "null"))
	null_or_real = pdf_new_null(ctx, doc);
      else
	{
	  null_or_real = pdfout_data_scalar_to_pdf_real (ctx, doc, scalar);
	}
      pdf_array_push (ctx, dest_array, null_or_real);
    }

  return dest_array;
}

static void
create_outline_dict (fz_context *ctx, pdf_document *doc, pdf_obj *dict,
		     pdfout_data *hash, pdf_obj *parent,
		     pdf_obj *prev, pdf_obj *next)
{
  /* Title.  */
  pdfout_data *value = pdfout_data_hash_gets (ctx, hash, "title");
  pdf_obj *title = pdfout_data_scalar_to_pdf_str (ctx, doc, value);
  pdf_dict_puts_drop (ctx, dict, "Title", title);

  /* Dest.  */
  const char *page_string = data_hash_get_string_key (ctx, hash, "page");
  int page = pdfout_strtoint_null (ctx, page_string);

  pdfout_data *view = pdfout_data_hash_gets (ctx, hash, "view");
  pdf_obj *dest_array = convert_dest_array (ctx, doc, view, page);
  pdf_dict_puts_drop (ctx, dict, "Dest", dest);

  /* Kids.  */
  pdfout_data *kids = pdofut_data_hash_gets (ctx, hash, "kids");
  if (kids)
    {
      pdf_obj *first, *last;
      create_outline_kids_array (ctx, doc, kids, dict, first, last);
      pdf_dict_puts_drop (ctx, dict, "First", first);
      pdf_dict_puts_drop (ctx, dict, "Last", last);
    }
  
  /* Parent.  */
  pdf_obj *parent_copy = copy_indirect_ref (ctx, doc, parent);
  pdf_dict_puts_drop (ctx, doc, dict, "Parent", parent_copy);

  /* Prev and Next. */
  if (prev)
    {
      pdf_obj *prev_copy = copy_indirect_ref (ctx, doc, prev);
      pdf_dict_puts_drop (ctx, dict, "Prev", prev_copy);
    }
  if (next)
    {
      pdf_obj *prev_copy = copy_indirect_ref (ctx, doc, prev);
      pdf_dict_puts_drop (ctx, dict, "Prev", prev_copy);
    }
  
    
}

static void
create_outline_kids_array (fz_context *ctx, pdf_document *doc,
			   pdfout_data *outline, pdf_obj *parent,
			   pdf_obj **first, pdf_obj **last)
{
  int len = pdfout_data_array_len (ctx, outline);

  /* Create a table of indirect references. To avoid circular references, all
     the created objects will make their own copies from these.  */
  pdf_obj **ref_table = fz_malloc_array (ctx, len, sizeof (pdf_obj *));
  pdf_obj **dict_table = fz_malloc_array (ctx, len, sizeof (pdf_obj *));
  for (int i = 0; i < len; ++i)
    {
      dict_table[i] = pdf_new_dict (ctx, doc, 8);
      ref_table[i] = pdf_add_object_drop (ctx, doc, dict_table[i]);
    }
  
  /* Populate objects.  */
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, outline, i);
      create_outline_dict (ctx, doc, dict_table[i], hash, parent,
			   i > 0 ? ref_table[i - 1] : NULL,
			   i < len - 1 ? ref_table[i + 1]: NULL);
    }
  *first = pdf_new_indirect (ctx, doc, pdf_to_num (ctx, ref_table[0]),
			     pdf_to_gen (ctx, ref_table[0]));
  *last = pdf_new_indirect (ctx, doc, pdf_to_num (ctx, ref_table[len - 1]),
			    pdf_to_gen (ctx, ref_table[len - 1]));

  /* Cleanup. */
  for (i = 0; i < length; ++i)
    free (ref_table[i]);
  free (ref_table);
  free (dict_table);
}

void
pdfout_outline_set (fz_context *ctx, pdf_document *doc, pdfout_data *outline)
{
  if (outline)
    check_outline (ctx, doc, outline);

  pdf_obj *root = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Root");
  if (root == NULL)
    pdfout_throw (ctx, "no document catalog, cannot update outline");

  /* Create new outline dict and write it into the Root dict.  */
  pdf_obj *dict = pdf_new_dict (ctx, doc, 4);
  pdf_obj *outline_ref = pdf_add_object_drop (ctx, doc, dict);
  pdf_dict_puts_drop (ctx, root, "Outlines", outline_ref);

  /* Populate the outline dict.  */
  pdf_obj *type_name = pdf_new_name (ctx, doc, "Outlines");
  pdf_dict_puts_drop (ctx, dict, "Type", type_name);

  if (outline == NULL)
    /* Remove outline.  */
    return;

  calculate_counts (ctx, outline);

  pdf_obj *first, *last;
  create_outline_kids_array (ctx, doc, outline, outline_ref, &first, &last);

  pdf_dict_puts_drop (ctx, outline, "First", first);
  pdf_dict_puts_drop (ctx, outline, "Last", last);
}

/* get outline */

static void
check_pdf_outline (fz_context *ctx, pdf_document *doc, pdf_obj *outline)
{
  do
    {
      if (pdf_mark_obj (ctx, outline))
	pdfout_throw (ctx, "circular reference");
      
      pdf_obj *title = pdf_dict_gets (ctx, outline, "Title");
      if (pdf_is_string (ctx, title) == false)
	pdfout_throw (ctx, "outline item without 'Title' key");
      
      pdf_obj *count = pdf_dict_gets (ctx, outline, "Count");
      if (count && pdf_is_int (ctx, count) == false)
	pdfout_throw (ctx, "value for key 'Count' not an integer");
      
      pdf_obj *dest = pdf_dict_gets (ctx, outline, "Dest");
      if (dest)
	check_dest (ctx, doc, dest);
      else
	fz_warn
    }
  while ((outline = pdf_dict_gets (ctx, outline, "Next")));
}

static pdfout_data *
get_outline_array (fz_context *ctx, pdf_document *doc, pdf_obj *dict)
{
  pdfout_data *result_array;
  
  
}

pdfout_data *
pdfout_outline_get (fz_context *ctx, pdf_document *doc)
{
  pdf_obj *root = pdf_dict_get (ctx, pdf_trailer (ctx, doc), PDF_NAME_Root);
  pdf_obj *outline_obj = pdf_dict_get (ctx, root, PDF_NAME_Outlines);
  pdf_obj *first = pdf_dict_get (ctx, obj, PDF_NAME_First);

  if (first)
    return outline_get (ctx, doc, first);
  else
    return pdfout_data_array_new (ctx);
}

