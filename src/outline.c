
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
  if (pdf_mark_obj (ctx, outline))
    pdfout_throw (ctx, "circular reference");
  
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

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("get outline: " fmt, ## args)
int
pdfout_outline_get (yaml_document_t **yaml_doc_ptr, fz_context *ctx,
		    pdf_document *doc)
{
  fz_outline *outline;
  int root;
  yaml_document_t *yaml_doc;
  bool have_error = false;

  outline = pdf_load_outline (ctx, doc);
  if (outline == NULL)
    {
      MSG ("document has no outline");
      have_error = true;
    }

  if (have_error == false)
    {
      
      *yaml_doc_ptr = yaml_doc = XZALLOC (yaml_document_t);
      pdfout_yaml_document_initialize (yaml_doc, NULL, NULL, NULL, 1, 1);
  
      root = fz_outline_to_yaml (ctx, outline, yaml_doc, 0);
      
      if (pdfout_sequence_length (yaml_doc, root) == 0)
	{
	  if (pdfout_batch_mode == 0)
	    MSG ("document has no outline");
	  have_error = 1;
	}
    }

  fz_drop_outline (ctx, outline);
  
  if (have_error)
    {
      *yaml_doc_ptr = NULL;
      return 1;
    }
  
  return 0;
      
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("convert outline to YAML: " fmt, ## args)

/* returns root node id  */
static int
fz_outline_to_yaml (fz_context *ctx, fz_outline *outline,
		    yaml_document_t *doc, int sequence)
{
  /* FIXME: add option to fail on unsupported link type */
  int value, mapping_id;
  char printf_buffer[256];
  yaml_scalar_style_t style;
  if (sequence == 0)		/* new level */
    sequence = pdfout_yaml_document_add_sequence (doc, NULL, 0);

  mapping_id = pdfout_yaml_document_add_mapping (doc, NULL, 0);
  style = strchr (outline->title, '\n') ? YAML_DOUBLE_QUOTED_SCALAR_STYLE
    : YAML_ANY_SCALAR_STYLE;
  value =
    pdfout_yaml_document_add_scalar (doc, NULL, outline->title, -1, style);
  pdfout_mapping_push_id (doc, mapping_id, "title", value);

  if (outline->dest.kind == FZ_LINK_GOTO)
    {
      pdfout_snprintf_old (printf_buffer, sizeof printf_buffer, "%d",
		       outline->dest.ld.gotor.page + 1);
      pdfout_mapping_push (doc, mapping_id, "page", printf_buffer);

      fz_point lt, rb;
      int flags;
      lt = outline->dest.ld.gotor.lt;
      rb = outline->dest.ld.gotor.rb;
      flags = outline->dest.ld.gotor.flags;
      value = parse_gotor (doc, flags, lt, rb);
      if (value > 0)
	pdfout_mapping_push_id (doc, mapping_id, "view", value);
      else
	MSG ("cannot parse link dest for title '%s'", outline->title);
    }
  else
    MSG ("skipping outline item with title '%s' as it is not a goto link"
	 " and this is not yet supported", outline->title);
  
  if (outline->is_open)
    pdfout_mapping_push (doc, mapping_id, "open", "true");

  if (outline->down)
    {				/* outline has children */
      value = fz_outline_to_yaml (ctx, outline->down, doc, 0);
      pdfout_mapping_push_id (doc, mapping_id, "kids", value);
    }
  pdfout_yaml_document_append_sequence_item (doc, sequence, mapping_id);

  if (outline->next)
    fz_outline_to_yaml (ctx, outline->next, doc, sequence);
  return sequence;
}

/* returns id of the generated sequence node  */
static int
parse_gotor (yaml_document_t *doc, int flags, fz_point lt, fz_point rb)
{
  /* FIXME: add option to fail on unknown gotor? */
  char buffer[256];
  int array =
    pdfout_yaml_document_add_sequence (doc, NULL, YAML_FLOW_SEQUENCE_STYLE);
  if (flags & fz_link_flag_r_is_zoom)
    {				/* XYZ */
      pdfout_sequence_push (doc, array, "XYZ");
      if (flags & fz_link_flag_l_valid)
	{
	  pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.x);
	  pdfout_sequence_push (doc, array, buffer);
	}
      else
	pdfout_sequence_push (doc, array, "null");
      if (flags & fz_link_flag_t_valid)
	{
	  pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.y);
	  pdfout_sequence_push (doc, array, buffer);
	}
      else
	pdfout_sequence_push (doc, array, "null");

      if (flags & fz_link_flag_r_valid)
	{
	  pdfout_snprintf_old (buffer, sizeof buffer, "%g", rb.x);
	  pdfout_sequence_push (doc, array, buffer);
	}
      else
	pdfout_sequence_push (doc, array, "null");
    }
  else if (flags == 060)
    pdfout_sequence_push (doc, array, "Fit");

  else if (flags == 020)
    {
      pdfout_sequence_push (doc, array, "FitH");
      pdfout_sequence_push (doc, array, "null");
    }
  else if (flags == 022)
    {
      pdfout_sequence_push (doc, array, "FitH");
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.y);
      pdfout_sequence_push (doc, array, buffer);
    }
  else if (flags == 040)
    {
      pdfout_sequence_push (doc, array, "FitV");
      pdfout_sequence_push (doc, array, "null");
    }
  else if (flags == 041)
    {
      pdfout_sequence_push (doc, array, "FitV");
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.x);
      pdfout_sequence_push (doc, array, buffer);
    }
  else if (flags == 077)
    {
      pdfout_sequence_push (doc, array, "FitR");
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.x);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", rb.y);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", rb.x);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf_old (buffer, sizeof buffer, "%g", lt.y);
      pdfout_sequence_push (doc, array, buffer);
    }
  else
    {
      MSG ("unknown flag '0%o' in parse_gotor", flags);
      return 0;
    }
  return array;
}

