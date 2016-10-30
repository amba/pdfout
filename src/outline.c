#include "common.h"

static bool
streq (const char *a, const char *b)
{
  return strcmp (a, b) == 0;
}

static const char *
data_hash_get_string_key (fz_context *ctx, pdfout_data *hash,
			  const char *key)
{
  pdfout_data *scalar = pdfout_data_hash_gets (ctx, hash, key);

  if (scalar == NULL)
    return NULL;
  

  int len;
  char *value = pdfout_data_scalar_get (ctx, scalar, &len);
  if (strlen (value) != len)
    pdfout_throw (ctx, "embedded null byte for key '%s'", key);
  return value;
}

static const char *
data_array_get_string (fz_context *ctx, pdfout_data *array, int i)
{
  pdfout_data *scalar = pdfout_data_array_get (ctx, array, i);
  int len;
  const char *result = pdfout_data_scalar_get (ctx, scalar, &len);
  if (len != strlen (result))
    pdfout_throw (ctx, "string with embedded null byte");
  return result;
}

static const char *
data_scalar_get_string (fz_context *ctx, pdfout_data *scalar)
{
  int len;
  const char *value = pdfout_data_scalar_get (ctx, scalar, &len);
  if (strlen (value) != len)
    pdfout_throw (ctx, "embedded null byte");
  return value;
}

static int
dest_sequence_length (fz_context *ctx, const char *label)
{
  if (streq (label, "XYZ"))
    return 4;
  else if (streq (label, "Fit") || streq (label, "FitB"))
    return 1;
  else if (streq (label, "FitH") || streq (label, "FitV")
	   || streq (label, "FitBH"))
    return 2;
  else if (streq (label, "FitR"))
    return 5;
  else
    pdfout_throw (ctx, "unknown destination: '%s'", label);
}

static void
check_sequence_numbers (fz_context *ctx, pdfout_data *dest)
{
  int len = pdfout_data_array_len (ctx, dest);
  for (int i = 1; i < len; ++i)
    {
      const char *num = data_array_get_string (ctx, dest, i);
      if (streq (num, "null"))
	continue;

      pdfout_strtof(ctx, num);
    }
}
			
static void
check_dest_sequence (fz_context *ctx, pdfout_data *dest)
{
  int len = pdfout_data_array_len (ctx, dest);
  if (len < 1 || len > 5)
    pdfout_throw (ctx, "illegal View sequence with length %d", len);

  const char *label = data_array_get_string (ctx, dest, 0);
  
  if (len != dest_sequence_length (ctx, label))
    pdfout_throw (ctx, "illegal %s sequence with length %d", label, len);

  check_sequence_numbers (ctx, dest);
}

static void
check_outline_array (fz_context *ctx, pdf_document *doc, pdfout_data *outline);

static void
check_outline_hash (fz_context *ctx, pdf_document *doc, pdfout_data *hash)
{
  int len = pdfout_data_hash_len (ctx, hash);
  bool has_title = false, has_page = false;
  
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *key = pdfout_data_hash_get_key (ctx, hash, i);
      pdfout_data *value = pdfout_data_hash_get_value (ctx, hash, i);
      if (pdfout_data_scalar_eq (ctx, key, "title"))
	{
	  if (pdfout_data_is_scalar (ctx, value) == false)
	    pdfout_throw (ctx, "value of key 'title' not a scalar");
	  has_title = true;
	}
      else if (pdfout_data_scalar_eq (ctx, key, "page"))
	{
	  const char *s = data_scalar_get_string (ctx, value);
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
      else if (pdfout_data_scalar_eq (ctx, key, "view"))
	check_dest_sequence (ctx, value);
      else if (pdfout_data_scalar_eq (ctx, key, "open"))
	{
	  if (pdfout_data_scalar_eq (ctx, value, "true") == false
	      && pdfout_data_scalar_eq (ctx, value, "false") == false)
	    pdfout_throw (ctx, "value of key 'open' not a bool");
	}
      else if (pdfout_data_scalar_eq (ctx, key, "kids"))
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
      int kid_count = calculate_kids_count (ctx, kid_hash);
      if (kid_count > 0)
	count += kid_count;
    }

  const char *open = data_hash_get_string_key (ctx, hash, "open");
  bool is_open = false;
  if (open && streq (open, "true"))
    is_open = true;

  if (is_open == false)
    count *= -1;

  char buf[200];
  pdfout_snprintf (ctx, buf, "%d", count);
  pdfout_data *key = pdfout_data_scalar_new (ctx, "count", strlen ("count"));
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
      calculate_kids_count (ctx, hash);
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

  pdfout_data *label = pdfout_data_array_get (ctx, view, 0);
  pdf_obj *label_obj = pdfout_data_scalar_to_pdf_name (ctx, doc, label);
  pdf_array_push (ctx, dest_array, label_obj);
  
  int len = pdfout_data_array_len (ctx, view);
  
  for (int i = 1; i < len; ++i)
    {
      pdf_obj *null_or_real;
      pdfout_data *scalar = pdfout_data_array_get (ctx, view, i);
      if (pdfout_data_scalar_eq (ctx, scalar, "null"))
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
create_outline_kids_array (fz_context *ctx, pdf_document *doc,
			   pdfout_data *outline, pdf_obj *parent,
			   pdf_obj **first, pdf_obj **last);

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
  pdf_dict_puts_drop (ctx, dict, "Dest", dest_array);

  /* Kids.  */
  pdfout_data *kids = pdfout_data_hash_gets (ctx, hash, "kids");
  if (kids)
    {
      pdf_obj *first, *last;
      create_outline_kids_array (ctx, doc, kids, dict, &first, &last);
      pdf_dict_puts_drop (ctx, dict, "First", first);
      pdf_dict_puts_drop (ctx, dict, "Last", last);

      /* Count.  */
      pdfout_data *count = pdfout_data_hash_gets (ctx, hash, "count");
      assert (count);
      pdf_obj *count_obj = pdfout_data_scalar_to_pdf_int (ctx, doc, count);
      pdf_dict_puts_drop (ctx, dict, "Count", count_obj);
    }
  
  /* Parent.  */
  pdf_obj *parent_copy = copy_indirect_ref (ctx, doc, parent);
  pdf_dict_puts_drop (ctx, dict, "Parent", parent_copy);

  /* Prev and Next. */
  if (prev)
    {
      pdf_obj *prev_copy = copy_indirect_ref (ctx, doc, prev);
      pdf_dict_puts_drop (ctx, dict, "Prev", prev_copy);
    }
  if (next)
    {
      pdf_obj *prev_copy = copy_indirect_ref (ctx, doc, next);
      pdf_dict_puts_drop (ctx, dict, "Next", prev_copy);
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
  for (int i = 0; i < len; ++i)
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

  pdf_dict_puts_drop (ctx, dict, "First", first);
  pdf_dict_puts_drop (ctx, dict, "Last", last);
}

/* Get outline.  */

static void
check_dest (fz_context *ctx, pdf_document *doc, pdf_obj *dest)
{
  dest = pdfout_resolve_dest(ctx, doc, dest, FZ_LINK_GOTO);
  if (dest == NULL)
    pdfout_throw (ctx, "undefined link kind");
  if (pdf_is_array (ctx, dest) == false)
    pdfout_throw (ctx, "destination is not an array object");
  int len = pdf_array_len (ctx, dest);

  pdf_obj *page_ref = pdf_array_get (ctx, dest, 0);
  int page = pdf_lookup_page_number (ctx, doc, page_ref);
  int page_count = pdf_count_pages (ctx, doc);
  if (page >= page_count)
    pdfout_throw (ctx, "page %d is exceeds page count %d", page, page_count);
  
  pdf_obj *name = pdf_array_get (ctx, dest, 1);
  char *name_str = pdf_to_name (ctx, name);
  if (name_str == NULL)
    pdfout_throw (ctx, "not a name object");

  int expected_len = 1 + dest_sequence_length (ctx, name_str);
  if (len != expected_len)
    pdfout_throw (ctx, "illegal %s dest array with length %d", name_str, len);
  for (int i = 2; i < len; ++i)
    {
      pdf_obj *obj = pdf_array_get (ctx, dest, i);
      if (pdf_is_null (ctx, obj) == false
	  && pdf_is_number (ctx, obj) == false)
	pdfout_throw (ctx, "illegal dest array");
    }

}

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

      pdf_obj *first = pdf_dict_gets (ctx, outline, "First");
      if (first)
	check_pdf_outline (ctx, doc, first);
      
    }
  while ((outline = pdf_dict_gets (ctx, outline, "Next")));
}

static void
data_hash_push_string_key (fz_context *ctx, pdfout_data *hash, const char *key,
			   pdfout_data *value)
{
  pdfout_data *key_data = pdfout_data_scalar_new (ctx, key, strlen (key));
  pdfout_data_hash_push (ctx, hash, key_data, value);
}

static pdfout_data *
data_scalar_from_int (fz_context *ctx, int number)
{
  char buf[200];
  int len = pdfout_snprintf (ctx, buf, "%d", number);
  return pdfout_data_scalar_new (ctx, buf, len);
}

static pdfout_data *
get_view_array (fz_context *ctx, pdf_document *doc, pdf_obj *dest, int *page)
{
  dest = pdfout_resolve_dest(ctx, doc, dest, FZ_LINK_GOTO);

  pdf_obj *page_ref = pdf_array_get (ctx, dest, 0);
  *page = pdf_lookup_page_number (ctx, doc, page_ref);

  pdfout_data *view_array = pdfout_data_array_new (ctx);

  pdf_obj *kind = pdf_array_get (ctx, dest, 1);
  pdfout_data *kind_data = pdfout_data_scalar_from_pdf (ctx, kind);
  pdfout_data_array_push (ctx, view_array, kind_data);

  int len = pdf_array_len (ctx, dest);
  
  for (int i = 2; i < len; ++i)
    {
      pdf_obj *item = pdf_array_get (ctx, dest, i);
      pdfout_data *item_data = pdfout_data_scalar_from_pdf (ctx, item);
      pdfout_data_array_push (ctx, view_array, item_data);
    }

  return view_array;
}

static pdfout_data *
get_outline_array (fz_context *ctx, pdf_document *doc, pdf_obj *outline);

static pdfout_data *
get_outline_hash (fz_context *ctx, pdf_document *doc, pdf_obj *outline)
{
  pdf_obj *title_obj = pdf_dict_gets (ctx, outline, "Title");
  pdfout_data *title = pdfout_data_scalar_from_pdf (ctx, title_obj);

  pdf_obj *dest = pdf_dict_gets (ctx, outline, "Dest");
  pdfout_data *view_array;

  int page;
  
  if (dest)
    view_array = get_view_array (ctx, doc, dest, &page);
  else
    {
      int title_len;
      char *title_str = pdfout_data_scalar_get (ctx, title, &title_len);
      pdfout_warn (ctx, "outline item with title '%s' has no destination",
		   title_str);
      pdfout_data_drop (ctx, title);
      return NULL;
    }
  
  
  pdfout_data *hash = pdfout_data_hash_new (ctx);
  
  
  data_hash_push_string_key (ctx, hash, "title", title);
  
  pdfout_data *page_value = data_scalar_from_int (ctx, page + 1);

  data_hash_push_string_key (ctx, hash, "page", page_value);
  
  data_hash_push_string_key (ctx, hash, "view", view_array);
  
  pdf_obj *count_obj = pdf_dict_gets (ctx, outline, "Count");
  if (count_obj)
    {
      int count_int = pdf_to_int (ctx, count_obj);
      const char *value = count_int > 0 ? "true" : "false";
      pdfout_data_hash_push_key_value (ctx, hash, "open", value,
				       strlen (value));
    }

  /* Kids.  */
  pdf_obj *first = pdf_dict_gets (ctx, outline, "First");
  if (first)
    {
      pdfout_data *kids_array = get_outline_array (ctx, doc, first);
      if (kids_array)
	data_hash_push_string_key (ctx, hash, "kids", kids_array);
    }

  return hash;
}

static pdfout_data *
get_outline_array (fz_context *ctx, pdf_document *doc, pdf_obj *outline)
{
  pdfout_data *result_array = pdfout_data_array_new (ctx);
  do
    {
      pdfout_data *hash = get_outline_hash (ctx, doc, outline);
      if (hash)
	pdfout_data_array_push (ctx, result_array, hash);
    }
  while ((outline = pdf_dict_gets (ctx, outline, "Next")));

  if (pdfout_data_array_len (ctx, result_array))
    return result_array;
  else
    {
      /* Empty list, return NULL.  */
      pdfout_data_drop (ctx, result_array);
      return NULL;
    }
}

pdfout_data *
pdfout_outline_get (fz_context *ctx, pdf_document *doc)
{
  pdf_obj *root = pdf_dict_get (ctx, pdf_trailer (ctx, doc), PDF_NAME_Root);
  pdf_obj *outline_obj = pdf_dict_get (ctx, root, PDF_NAME_Outlines);
  pdf_obj *first = pdf_dict_get (ctx, outline_obj, PDF_NAME_First);

  if (first)
    {
      check_pdf_outline (ctx, doc, first);
      return get_outline_array (ctx, doc, first);
    }
  else
    return pdfout_data_array_new (ctx);
}

