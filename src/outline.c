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
#include "outline-wysiwyg.h"
#include "libyaml-wrappers.h"

static int fz_outline_to_yaml (fz_context *ctx, fz_outline *outline,
			       yaml_document_t *doc, int sequence);
static void convert_yaml_sequence_to_outline (fz_context *ctx,
					      yaml_document_t *yaml_doc,
					      pdf_document *doc,
					      int sequence, pdf_obj *Parent,
					      pdf_obj **First,
					      pdf_obj **Last);
static void convert_yaml_dest (fz_context *ctx, yaml_document_t *yaml_doc,
			       pdf_document *doc, yaml_node_t *node,
			       pdf_obj *dest);
static int check_yaml_outline (pdf_document *doc, yaml_document_t *yaml_doc);
static int check_yaml_outline_sequence_node (pdf_document *doc,
					     yaml_document_t *yaml_doc,
					     int sequence);
static int check_yaml_outline_mapping_node (pdf_document *doc,
					    yaml_document_t *yaml_doc,
					    int mapping);
static int check_yaml_dest_sequence (yaml_document_t *doc,
				     yaml_node_t *node);
static int check_yaml_dest_sequence_numbers (yaml_document_t *doc,
					     yaml_node_t *node);
static void calculate_counts (yaml_document_t *doc);
static int calculate_node_count (yaml_document_t *doc, int node_id);
static yaml_node_t *create_default_dest_sequence (yaml_document_t *doc,
						  int mapping_id);
static int parse_gotor (yaml_document_t *doc, int flags, fz_point lt,
			fz_point rb);

yaml_document_t *pdfout_default_dest_array;

int
pdfout_outline_load (yaml_document_t **doc, FILE *input,
		     enum pdfout_outline_format format)
{
  if (format == PDFOUT_OUTLINE_YAML)
    return pdfout_load_yaml (doc, input);
  else if (format == PDFOUT_OUTLINE_WYSIWYG)
    return pdfout_outline_wysiwyg_to_yaml (doc, input);
  else
    error (1, 0, "pdfout_outline_load: invalid format");

  return 0;
}

int
pdfout_outline_dump (FILE *output, yaml_document_t *doc,
		     enum pdfout_outline_format format)
{
  if (format == PDFOUT_OUTLINE_YAML)
    {
      pdfout_dump_yaml (output, doc);
      return 0;
    }
  else if (format == PDFOUT_OUTLINE_WYSIWYG)
    return pdfout_outline_dump_to_wysiwyg (output, doc);
  else
    error (1, 0, "pdfout_outline_dump: invalid format");

  return 0;
}

/* set outline */

#define MSG(fmt, args...) pdfout_msg ("set outline: " fmt, ## args)

int
pdfout_outline_set (fz_context *ctx, pdf_document *doc,
		    yaml_document_t *yaml_doc)
{
  pdf_obj *outline, *dict, *root, *First, *Last;
  yaml_node_t *node;
  int length;

  if (yaml_doc)
    {
      /*initialize doc->page_count */
      pdf_count_pages (ctx, doc);
      
      if (check_yaml_outline (doc, yaml_doc))
	return 1;
    }
  
  root = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Root");
  if (root == NULL)
    {
      MSG ("no document catalog, cannot update outline");
      return 2;
    }

  dict = pdf_new_dict (ctx, doc, 4);
  pdf_dict_puts_drop (ctx, dict, "Type", pdf_new_name (ctx, doc, "Outlines"));
  outline = pdf_new_ref (ctx, doc, dict);
  pdf_drop_obj (ctx, dict);
  pdf_dict_puts_drop (ctx, root, "Outlines", outline);

  if (yaml_doc)
    node = yaml_document_get_node (yaml_doc, 1);
  
  if (yaml_doc == NULL || node == NULL)
    {
      MSG ("removing outline");
      return 0;
    }
  assert (node->type == YAML_SEQUENCE_NODE);
  length = pdfout_sequence_length (yaml_doc, 1);
  if (length == 0)
    {
      MSG ("no input (empty YAML root sequence), removing outline");
      return 0;
    }
  calculate_counts (yaml_doc);
  convert_yaml_sequence_to_outline (ctx, yaml_doc, doc, 1, outline, &First,
				    &Last);
  assert (First && Last);
  pdf_dict_puts_drop (ctx, outline, "First", First);
  pdf_dict_puts_drop (ctx, outline, "Last", Last);

  return 0;
}

static void
convert_yaml_sequence_to_outline (fz_context *ctx, yaml_document_t *yaml_doc,
				  pdf_document *doc, int sequence,
				  pdf_obj *Parent, pdf_obj **First,
				  pdf_obj **Last)
{
  pdf_obj *dict, *value, *page_ref, *dest, *Kids_First, *Kids_Last,
    **ref_table, **dict_table;
  yaml_node_t *node;
  int i, length, page, Count, mapping, kids;
  size_t title_len;
  char *title;
  length = pdfout_sequence_length (yaml_doc, sequence);
  assert (length > 0);
  /* allocate objects */
  ref_table = xnmalloc (length, sizeof (pdf_obj *));
  dict_table = xnmalloc (length, sizeof (pdf_obj *));
  for (i = 0; i < length; ++i)
    {
      dict_table[i] = pdf_new_dict (ctx, doc, 8);
      ref_table[i] = pdf_new_ref (ctx, doc, dict_table[i]);
      pdf_drop_obj (ctx, dict_table[i]);
    }

  /* populate objects */
  for (i = 0; i < length; ++i)
    {
      mapping = pdfout_sequence_get (yaml_doc, sequence, i);
      assert (mapping);
      dict = dict_table[i];

      /* Title */
      node = pdfout_mapping_gets_node (yaml_doc, mapping, "title");
      assert (node && node->type == YAML_SCALAR_NODE
	      && pdfout_scalar_value (node));
      title = pdfout_utf8_to_str (pdfout_scalar_value (node),
				  node->data.scalar.length, &title_len);
      value = pdf_new_string (ctx, doc, title, title_len);
      free (title);
      pdf_dict_puts_drop (ctx, dict, "Title", value);

      /* Parent */
      pdf_dict_puts_drop
	(ctx, dict, "Parent", pdf_new_indirect
	 (ctx, doc, pdf_to_num (ctx, Parent), pdf_to_gen (ctx, Parent)));
      
      /* Prev */
      if (i > 0)
	pdf_dict_puts_drop (ctx, dict, "Prev", pdf_new_indirect
			    (ctx, doc, pdf_to_num (ctx, ref_table[i - 1]),
			     pdf_to_gen (ctx, ref_table[i - 1])));
      /* Next */
      if (i < length - 1)
	pdf_dict_puts_drop (ctx, dict, "Next", pdf_new_indirect
			    (ctx, doc, pdf_to_num (ctx, ref_table[i + 1]),
			     pdf_to_gen (ctx, ref_table[i + 1])));

      /* Dest */
      dest = pdf_new_array (ctx, doc, 6);
      node = pdfout_mapping_gets_node (yaml_doc, mapping, "page");
      assert (node && node->type == YAML_SCALAR_NODE
	      && pdfout_scalar_value (node));
      page = pdfout_strtoint_null (pdfout_scalar_value (node));
      page_ref = pdf_lookup_page_obj (ctx, doc, page - 1);
      assert (page_ref);
      pdf_array_push (ctx, dest, page_ref);
      node = pdfout_mapping_gets_node (yaml_doc, mapping, "view");
      assert (node);
      convert_yaml_dest (ctx, yaml_doc, doc, node, dest);
      pdf_dict_puts_drop (ctx, dict, "Dest", dest);

      /* Kids */
      kids = pdfout_mapping_gets (yaml_doc, mapping, "kids");
      if (kids)
	{
	  convert_yaml_sequence_to_outline (ctx, yaml_doc, doc, kids,
					    ref_table[i], &Kids_First,
					    &Kids_Last);
	  assert (Kids_First && Kids_Last);
	  pdf_dict_puts_drop (ctx, dict, "First", Kids_First);
	  pdf_dict_puts_drop (ctx, dict, "Last", Kids_Last);

	  /* Count */
	  node = pdfout_mapping_gets_node (yaml_doc, mapping, "Count");
	  assert (node && node->type == YAML_SCALAR_NODE);
	  Count = pdfout_strtoint_null (pdfout_scalar_value (node));
	  pdf_dict_puts_drop (ctx, dict, "Count",
			      pdf_new_int (ctx, doc, Count));
	}
    }

  /* return First and Last to caller */
  *First = pdf_new_indirect (ctx, doc, pdf_to_num (ctx, ref_table[0]),
			     pdf_to_gen (ctx, ref_table[0]));
  *Last = pdf_new_indirect (ctx, doc, pdf_to_num (ctx, ref_table[length - 1]),
			    pdf_to_gen (ctx, ref_table[length - 1]));
  /* cleanup */
  for (i = 0; i < length; ++i)
    free (ref_table[i]);
  free (ref_table);
  free (dict_table);
}

static void
convert_yaml_dest (fz_context *ctx, yaml_document_t *yaml_doc,
		   pdf_document *doc, yaml_node_t *node, pdf_obj *dest)
{
  yaml_node_t *scalar;
  float real;
  int length, i;
  assert (node->type = YAML_SEQUENCE_NODE && node->data.sequence.items.start
	  != node->data.sequence.items.top);
  length = node->data.sequence.items.top - node->data.sequence.items.start;
  assert (length > 0);
  scalar =
    yaml_document_get_node (yaml_doc, *node->data.sequence.items.start);
  assert (scalar && scalar->type == YAML_SCALAR_NODE);
  pdf_array_push_drop (ctx, dest,
		       pdf_new_name (ctx, doc, pdfout_scalar_value (scalar)));
  for (i = 1; i < length; ++i)
    {
      scalar =
	yaml_document_get_node (yaml_doc,
				*node->data.sequence.items.start + i);
      assert (scalar && scalar->type == YAML_SCALAR_NODE);
      if (pdfout_is_null (pdfout_scalar_value (scalar)))
	pdf_array_push_drop (ctx, dest, pdf_new_null (ctx, doc));
      else
	{
	  real = pdfout_strtof (pdfout_scalar_value (scalar));
	  pdf_array_push_drop (ctx, dest, pdf_new_real (ctx, doc, real));
	}
    }
}

static void
calculate_counts (yaml_document_t *doc)
{
  int length, i;
  yaml_node_t *root = yaml_document_get_node (doc, 1);
  if (root == NULL)
    return;
  assert (root->type == YAML_SEQUENCE_NODE);
  length = pdfout_sequence_length (doc, 1);
  for (i = 0; i < length; ++i)
    calculate_node_count (doc, pdfout_sequence_get (doc, 1, i));
}

static int
calculate_node_count (yaml_document_t *doc, int node_id)
{
  int count, kids, num_kids, i, kid_count, open;
  yaml_node_t *open_node; /* *node; */
  char printf_buffer[20];
  open = 0;
  count = 0;
  /* node = yaml_document_get_node (doc, node_id); */
  /* assert (node && node->type == YAML_MAPPING_NODE); */
  kids = pdfout_mapping_gets (doc, node_id, "kids");
  if (kids)
    {
      assert (yaml_document_get_node (doc, kids)->type == YAML_SEQUENCE_NODE);
      num_kids = pdfout_sequence_length (doc, kids);
      assert (num_kids > 0);
      for (i = 0; i < num_kids; ++i)
	{
	  kid_count =
	    calculate_node_count (doc, pdfout_sequence_get (doc, kids, i));
	  if (kid_count <= 0)
	    ++count;
	  else
	    count += kid_count + 1;
	}

      open_node = pdfout_mapping_gets_node (doc, node_id, "open");
      if (open_node)
	{
	  assert (open_node->type == YAML_SCALAR_NODE);
	  if (pdfout_is_true (pdfout_scalar_value (open_node)))
	    open = 1;
	}
      if (open == 0)
	count *= -1;
      pdfout_snprintf (printf_buffer, sizeof printf_buffer, "%d", count);
      pdfout_mapping_push (doc, node_id, "Count", printf_buffer);
    }
  return count;
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("check outline: " fmt, ## args);


static int
check_yaml_outline (pdf_document *doc, yaml_document_t *yaml_doc)
{
  yaml_node_t *node = yaml_document_get_root_node (yaml_doc);
  int length;
  if (node == NULL)
    {
      MSG ("empty YAML document");
      return 0;
    }
  if (node->type != YAML_SEQUENCE_NODE)
    {
      MSG ("not a sequence node");
      return -1;
    }
  length = pdfout_sequence_length (yaml_doc, 1);
  if (length == 0)
    {
      MSG ("empty root sequence");
      return 0;
    }
  return check_yaml_outline_sequence_node (doc, yaml_doc, 1);
}

static int
check_yaml_outline_sequence_node (pdf_document *doc,
				  yaml_document_t *yaml_doc, int sequence)
{
  int mapping, length, i;
  yaml_node_t *node;
  node = yaml_document_get_node (yaml_doc, sequence);
  if (node->type != YAML_SEQUENCE_NODE)
    {
      MSG ("wrong node type in YAML document");
      return -1;
    }
  length = pdfout_sequence_length (yaml_doc, sequence);
  if (length < 1)
    {
      MSG ("empty sequence");
      return -1;
    }
  for (i = 0; i < length; ++i)
    {
      mapping = pdfout_sequence_get (yaml_doc, sequence, i);
      assert (mapping);
      if (check_yaml_outline_mapping_node (doc, yaml_doc, mapping))
	return -1;
    }
  return 0;
}

static int
check_yaml_outline_mapping_node (pdf_document *doc,
				 yaml_document_t *yaml_doc, int mapping_id)
{
  yaml_node_t *node, *mapping;
  char *title, *endptr;
  int page, kids;
  mapping = yaml_document_get_node (yaml_doc, mapping_id);
  assert (mapping);
  if (mapping->type != YAML_MAPPING_NODE)
    {
      MSG ("wrong node type in YAML document");
      return -1;
    }
  node = pdfout_mapping_gets_node (yaml_doc, mapping_id, "title");
  if (node == NULL)
    {
      MSG ("outline item without title");
      return -1;
    }
  title = pdfout_scalar_value (node);
  node = pdfout_mapping_gets_node (yaml_doc, mapping_id, "page");
  if (node == NULL || node->type != YAML_SCALAR_NODE
      || pdfout_scalar_value (node) == NULL)
    {
      MSG ("no page number for title '%s'", title);
      return -1;
    }
  page = pdfout_strtoint (pdfout_scalar_value (node), &endptr);
  if (pdfout_scalar_value (node) == endptr)
    {
      MSG ("invalid page number: '%s'", pdfout_scalar_value (node));
      return -1;
    }
  if (page < 1)
    {
      MSG ("page number %d for title '%s' not a positive integer",
		 page, title);
      return -1;
    }
  if (page > doc->page_count)
    {
      MSG ("page number %d for title '%s' is greater than pagecount %d",
		  page, title, doc->page_count);
      return -1;
    }
  node = pdfout_mapping_gets_node (yaml_doc, mapping_id, "view");
  if (node == NULL)
    node = create_default_dest_sequence (yaml_doc, mapping_id);
  if (check_yaml_dest_sequence (yaml_doc, node))
    {
      MSG ("value of key 'view' broken for title '%s'", title);
      return -1;
    }

  node = pdfout_mapping_gets_node (yaml_doc, mapping_id, "open");
  if (node)
    {
      if (node->type != YAML_SCALAR_NODE)
	{
	  MSG ("value of key 'open' for title '%s' not a scalar", title);
	  return -1;
	}
      if (pdfout_is_bool (pdfout_scalar_value (node)) == 0)
	{
	  MSG ("value of key 'open' for title '%s' not a YAML bool",
		     pdfout_scalar_value (node));
	  return -1;
	}
    }
  kids = pdfout_mapping_gets (yaml_doc, mapping_id, "kids");
  if (kids)
    if (check_yaml_outline_sequence_node (doc, yaml_doc, kids))
      return -1;

  return 0;
}

static yaml_node_t *
create_default_dest_sequence (yaml_document_t *doc, int mapping)
{
  int sequence;
  int i, length;
  yaml_node_t *array, *scalar;
  sequence =
    pdfout_yaml_document_add_sequence (doc, 0, YAML_FLOW_SEQUENCE_STYLE);
  if (pdfout_default_dest_array == NULL)
    {
      pdfout_sequence_push (doc, sequence, "XYZ");
      pdfout_sequence_push (doc, sequence, "null");
      pdfout_sequence_push (doc, sequence, "null");
      pdfout_sequence_push (doc, sequence, "null");
      pdfout_mapping_push_id (doc, mapping, "view", sequence);
      return yaml_document_get_node (doc, sequence);
    }
  array = yaml_document_get_root_node (pdfout_default_dest_array);
  if (array == NULL || array->type != YAML_SEQUENCE_NODE)
    {
      pdfout_msg ("default view not a sequence");
      return NULL;
    }
  length = pdfout_sequence_length (pdfout_default_dest_array, 1);
  for (i = 0; i < length; ++i)
    {
      scalar = pdfout_sequence_get_node (pdfout_default_dest_array, 1, i);
      assert (scalar);
      if (scalar->type != YAML_SCALAR_NODE)
	{
	  pdfout_msg ("node in default view not a scalar");
	  return NULL;
	}
      pdfout_sequence_push (doc, sequence, pdfout_scalar_value (scalar));
    }
  pdfout_mapping_push_id (doc, mapping, "view", sequence);
  return yaml_document_get_node (doc, sequence);
}

#undef MSG
#define MSG(fmt, args...) \
  pdfout_msg ("check outline: 'view' key: " fmt, ## args)

static int
check_yaml_dest_sequence (yaml_document_t *doc, yaml_node_t *node)
{
  int length, *item_id;
  yaml_node_t *label_item, *item;
  char *label, *value;

  if (node == NULL || node->type != YAML_SEQUENCE_NODE)
    {
      MSG ("destination is not a sequence");
      return -1;
    }
  length = node->data.sequence.items.top - node->data.sequence.items.start;
  if (length < 1 || length > 5)
    {
      MSG ("illegal View sequence with length %d", length);
      return -1;
    }
  label_item = yaml_document_get_node (doc, *node->data.sequence.items.start);

  if (label_item->type != YAML_SCALAR_NODE
      || pdfout_scalar_value (label_item) == NULL)
    {
      MSG ("label not a scalar");
      return -1;
    }
  label = pdfout_scalar_value (label_item);
  if (strcmp (label, "XYZ") == 0)
    {
      if (length != 4)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      if (check_yaml_dest_sequence_numbers (doc, node))
	return -1;
    }
  else if (strcmp (label, "Fit") == 0)
    {
      if (length != 1)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
    }
  else if (strcmp (label, "FitH") == 0)
    {
      if (length != 2)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      if (check_yaml_dest_sequence_numbers (doc, node))
	return -1;
    }
  else if (strcmp (label, "FitV") == 0)
    {
      if (length != 2)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      if (check_yaml_dest_sequence_numbers (doc, node))
	return -1;
    }
  else if (strcmp (label, "FitR") == 0)
    {
      /* no null allowed here */
      if (length != 5)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      for (item_id = node->data.sequence.items.start + 1;
	   item_id < node->data.sequence.items.top; ++item_id)
	{
	  item = yaml_document_get_node (doc, *item_id);
	  if (item == NULL || item->type != YAML_SCALAR_NODE
	      || pdfout_scalar_value (item) == NULL)
	    {
	      MSG ("illegal destination");
	      return -1;
	    }
	  value = pdfout_scalar_value (item);
	  if (isnan (pdfout_strtof_nan (value)))
	    {
	      MSG ("illegal destination");
	      return -1;
	    }
	}
    }
  else if (strcmp (label, "FitB") == 0)
    {
      if (length != 1)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
    }
  else if (strcmp (label, "FitBH") == 0)
    {
      if (length != 2)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      if (check_yaml_dest_sequence_numbers (doc, node))
	return -1;
    }
  else if (strcmp (label, "FitBV") == 0)
    {
      if (length != 2)
	{
	  MSG ("illegal %s sequence with length %d", label, length);
	  return -1;
	}
      if (check_yaml_dest_sequence_numbers (doc, node))
	return -1;
    }
  else
    {
      MSG ("unknown destination: '%s'", label);
      return -1;
    }
  return 0;
}

static int
check_yaml_dest_sequence_numbers (yaml_document_t *doc, yaml_node_t *node)
{
  int *item_id;
  yaml_node_t *item;
  char *value;
  for (item_id = node->data.sequence.items.start + 1;
       item_id < node->data.sequence.items.top; ++item_id)
    {
      item = yaml_document_get_node (doc, *item_id);
      if (item == NULL || item->type != YAML_SCALAR_NODE
	  || pdfout_scalar_value (item) == NULL)
	{
	  MSG ("illegal destination");
	  return -1;
	}
      value = pdfout_scalar_value (item);
      if (pdfout_is_null (value))
	continue;
      if (isnan (pdfout_strtof_nan (value)))
	{
	  MSG ("not a float: \"%s\"", value);
	  return -1;
	}
    }
  return 0;
}

/* get outline */

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
      pdfout_snprintf (printf_buffer, sizeof printf_buffer, "%d",
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
	  pdfout_snprintf (buffer, sizeof buffer, "%g", lt.x);
	  pdfout_sequence_push (doc, array, buffer);
	}
      else
	pdfout_sequence_push (doc, array, "null");
      if (flags & fz_link_flag_t_valid)
	{
	  pdfout_snprintf (buffer, sizeof buffer, "%g", lt.y);
	  pdfout_sequence_push (doc, array, buffer);
	}
      else
	pdfout_sequence_push (doc, array, "null");

      if (flags & fz_link_flag_r_valid)
	{
	  pdfout_snprintf (buffer, sizeof buffer, "%g", rb.x);
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
      pdfout_snprintf (buffer, sizeof buffer, "%g", lt.y);
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
      pdfout_snprintf (buffer, sizeof buffer, "%g", lt.x);
      pdfout_sequence_push (doc, array, buffer);
    }
  else if (flags == 077)
    {
      pdfout_sequence_push (doc, array, "FitR");
      pdfout_snprintf (buffer, sizeof buffer, "%g", lt.x);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf (buffer, sizeof buffer, "%g", rb.y);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf (buffer, sizeof buffer, "%g", rb.x);
      pdfout_sequence_push (doc, array, buffer);
      pdfout_snprintf (buffer, sizeof buffer, "%g", lt.y);
      pdfout_sequence_push (doc, array, buffer);
    }
  else
    {
      MSG ("unknown flag '0%o' in parse_gotor", flags);
      return 0;
    }
  return array;
}

