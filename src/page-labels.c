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

#define MSG(fmt, args...) pdfout_msg ("check page labels: " fmt, ## args)

/* returns non-zero if page labels in YAML_DOC are invalid */
static int
check_page_labels (fz_context *ctx, pdf_document *doc,
		   yaml_document_t *yaml_doc)
{
  yaml_node_t *sequence, *mapping, *scalar;
  char *endptr, *string;
  int length, i, mapping_id, number;
  
  pdf_count_pages (ctx, doc);
  sequence = yaml_document_get_root_node (yaml_doc);
  if (sequence == NULL)
    {
      MSG ("empty YAML document");
      return 0;
    }
  if (sequence->type != YAML_SEQUENCE_NODE)
    {
      MSG ("not a sequence");
      return 1;
    }
  length = pdfout_sequence_length (yaml_doc, 1);

  for (i = 0; i < length; ++i)
    {
      mapping_id = pdfout_sequence_get (yaml_doc, 1, i);
      mapping = yaml_document_get_node (yaml_doc, mapping_id);
      if (mapping == NULL || mapping->type != YAML_MAPPING_NODE)
	{
	  MSG ("missing mapping node");
	  return 1;
	}
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping_id, "page");
      if (scalar == NULL || scalar->type != YAML_SCALAR_NODE)
	{
	  MSG ("missing required key 'page'");
	  return 1;
	}

      string = pdfout_scalar_value (scalar);
      number = pdfout_strtoint (string, &endptr);
      if (endptr == string || endptr[0] != '\0')
	{
	  MSG ("invalid page string '%s'", string);
	  return 1;
	}

      if (number < 1 || number > doc->page_count)
	{
	  MSG ("page %d out of range 1..%d", number,
			   doc->page_count);
	  return 1;
	}

      if (i == 0 && number != 1)
	{
	  MSG ("missing required page label for page 1");
	  return 1;
	}

      scalar = pdfout_mapping_gets_node (yaml_doc, mapping_id, "style");
      if (scalar)
	{
	  if (scalar->type != YAML_SCALAR_NODE)
	    {
	      MSG ("value of key 'style' not a scalar node");
	      return 1;
	    }
	  string = pdfout_scalar_value (scalar);
	  if (strcasecmp (string, "arabic") && strcmp (string, "Roman")
	      && strcmp (string, "roman") && strcmp (string, "letters")
	      && strcmp (string, "Letters"))
	    {
	      MSG ("unknown value '%s' of key 'style'.\n"
		   "supported values are arabic,Roman,roman,letters,Letters.",
		   string);
	      return 1;
	    }
	}
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping_id, "first");
      if (scalar)
	{
	  if (scalar->type != YAML_SCALAR_NODE)
	    {
	      MSG ("value of key 'first' not a scalar node");
	      return 1;
	    }
	  string = pdfout_scalar_value (scalar);
	  number = pdfout_strtoint (string, &endptr);
	  if (endptr == string || endptr[0] != '\0')
	    {
	      MSG ("value '%s' of key 'first' not a number",
			       string);
	      return 1;
	    }
	  if (number < 1)
	    {
	      MSG ("value of key 'first' has to be >= 1");
	      return 1;
	    }
	}
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping_id, "prefix");
      if (scalar)
	{
	  if (scalar->type != YAML_SCALAR_NODE)
	    {
	      MSG ("value of key 'prefix' not a scalar");
	      return 1;
	    }
	}
    }
  return 0;
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("update page labels: " fmt, ## args)

int
pdfout_update_page_labels (fz_context *ctx, pdf_document *doc,
			   yaml_document_t *yaml_doc)
{
  int length, i;
  pdf_obj *root, *labels, *array, *dict;

  if (yaml_doc)
    {
      /* initialize doc->page_count */
      pdf_count_pages (ctx, doc);
      if (check_page_labels (ctx, doc, yaml_doc))
	return 1;
    }
  
  root = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Root");

  if (root == NULL)
    {
      MSG ("no document catalog, cannot update page labels");
      return 1;
    }

  labels = pdf_new_dict (ctx, doc, 1);
  array = pdf_new_array (ctx, doc, 2);
  pdf_dict_puts_drop (ctx, labels, "Nums", array);
  pdf_dict_puts_drop (ctx, root, "PageLabels", labels);

  if (yaml_doc)
    length = pdfout_sequence_length (yaml_doc, 1);
  
  if (yaml_doc == NULL || length == 0)
    {
      MSG ("removing page labels");
      return 0;
    }

  for (i = 0; i < length; ++i)
    {
      yaml_node_t *scalar;
      int mapping, number;
      
      dict = pdf_new_dict (ctx, doc, 3);
      mapping = pdfout_sequence_get (yaml_doc, 1, i);
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping, "page");
      assert (scalar && scalar->type == YAML_SCALAR_NODE);
      number = pdfout_strtoint_null (pdfout_scalar_value (scalar)) - 1;
      pdf_array_push_drop (ctx, array, pdf_new_int (ctx, doc, number));
      pdf_array_push_drop (ctx, array, dict);
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping, "style");
      if (scalar)
	{
          char *string;

	  string = pdfout_scalar_value (scalar);
	  if (strcmp (string, "arabic") == 0)
	    pdf_dict_puts_drop (ctx, dict, "S", pdf_new_name (ctx, doc, "D"));
	  else if (strcmp (string, "Roman") == 0)
	    pdf_dict_puts_drop (ctx, dict, "S", pdf_new_name (ctx, doc, "R"));
	  else if (strcmp (string, "roman") == 0)
	    pdf_dict_puts_drop (ctx, dict, "S", pdf_new_name (ctx, doc, "r"));
	  else if (strcmp (string, "letters") == 0)
	    pdf_dict_puts_drop (ctx, dict, "S", pdf_new_name (ctx, doc, "a"));
	  else if (strcmp (string, "Letters") == 0)
	    pdf_dict_puts_drop (ctx, dict, "S", pdf_new_name (ctx, doc, "A"));
	  else
	    /* should never happen  */
	    error (1, 0, "unknown style %s", string);
	}
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping, "first");
      if (scalar)
	{
	  number = pdfout_strtoint_null (pdfout_scalar_value (scalar));
	  pdf_dict_puts_drop (ctx, dict, "St", pdf_new_int (ctx, doc, number));
	}
      scalar = pdfout_mapping_gets_node (yaml_doc, mapping, "prefix");
      if (scalar)
	{
	  char *text_string;
	  int text_string_len;

	  text_string =
	    pdfout_utf8_to_text_string (pdfout_scalar_value (scalar),
					scalar->data.scalar.length,
					&text_string_len);
	  pdf_dict_puts_drop (ctx, dict, "P",
			      pdf_new_string (ctx, doc, text_string,
					      text_string_len));
	  free (text_string);
	}
    }
  return 0;
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("get page labels: " fmt, ## args)

/* return values like for pdfout_get_page_labels () */
static int
get_page_labels (yaml_document_t *yaml_doc, fz_context *ctx,
		 pdf_document *doc)
{
  pdf_obj *array, *labels;
  int length, i, sequence;
  
  labels = pdf_dict_getp (ctx, pdf_trailer (ctx, doc), "Root/PageLabels");
  
  if (labels == NULL)
    {
      MSG ("no 'PageLabels' entry found in the document catalogue");
      return 1;
    }
  
  array = pdf_dict_gets (ctx, labels, "Nums");
  length = pdf_array_len (ctx, array);
  
  if (length == 0)
    {
      MSG ("empty 'PageLabels' entry");
      return 1;
    }

  if (length % 2)
    MSG ("uneven number in Nums array");
  
  sequence = pdfout_yaml_document_add_sequence (yaml_doc, NULL, 0);
  for (i = 0; i < length / 2; ++i)
    {
      pdf_obj *dict, *key, *P;
      int  index, mapping, St, pdf_buf_len, utf8_buf_len;
      char *style, *S, *pdf_buf, *utf8_buf;
      char buffer[30];
      
      key = pdf_array_get (ctx, array, 2 * i);
      
      if (pdf_is_int (ctx, key) == 0)
	{
	  MSG ("key in number tree not an int");
	  continue;
	}
      
      index = pdf_to_int (ctx, key);
      
      if (index < 0)
	{
	  MSG ("key in number tree is < 0");
	  continue;
	}
      
      mapping = pdfout_yaml_document_add_mapping (yaml_doc, NULL, 0);
      pdfout_yaml_document_append_sequence_item (yaml_doc, sequence, mapping);
      pdfout_snprintf (buffer, sizeof buffer, "%d", index + 1);
      pdfout_mapping_push (yaml_doc, mapping, "page", buffer);
      
      dict = pdf_array_get (ctx, array, 2 * i + 1);

      /* pdf_to_name returns the empty string - not NULL - on all errors,
	 so the below strcmp () is allowed.  */
      S = pdf_to_name (ctx, pdf_dict_gets (ctx, dict, "S"));
      style = NULL;
      if (strcmp (S, "D") == 0)
	style = "arabic";
      else if (strcmp (S, "R") == 0)
	style = "Roman";
      else if (strcmp (S, "r") == 0)
	style = "roman";
      else if (strcmp (S, "A") == 0)
	style = "Letters";
      else if (strcmp (S, "a") == 0)
	style = "letters";
      if (style)
	pdfout_mapping_push (yaml_doc, mapping, "style", style);
      /* no else here, since the 'S' key is optional  */
      
      St = pdf_to_int (ctx, pdf_dict_gets (ctx, dict, "St"));
      if (St)
	{
	  pdfout_snprintf (buffer, sizeof buffer, "%d", St);
	  pdfout_mapping_push (yaml_doc, mapping, "first", buffer);
	}
      
      P = pdf_dict_gets (ctx, dict, "P");
      if (P)
	{
	  pdf_buf = pdf_to_str_buf (ctx, P);
	  pdf_buf_len = pdf_to_str_len (ctx, P);
	  if (pdf_buf_len)
	    {
	      utf8_buf =
		pdfout_text_string_to_utf8 (pdf_buf, pdf_buf_len,
					    &utf8_buf_len);
	      pdfout_mapping_push (yaml_doc, mapping, "prefix", utf8_buf);
	      free (utf8_buf);
	    }
	}

    }
  
  if (pdfout_sequence_length (yaml_doc, sequence) == 0)
    return 1;
  
  return 0;
}

int
pdfout_get_page_labels (yaml_document_t **yaml_doc_ptr, fz_context *ctx,
			pdf_document *doc)
{
  yaml_document_t *yaml_doc;

  *yaml_doc_ptr = yaml_doc = XZALLOC (yaml_document_t);

  pdfout_yaml_document_initialize (yaml_doc, NULL, NULL, NULL, 1, 1);

  if (get_page_labels (yaml_doc, ctx, doc))
    {
      yaml_document_delete (yaml_doc);
      free (yaml_doc);
      *yaml_doc_ptr = NULL;
      return 1;
    }

  return 0;
}
