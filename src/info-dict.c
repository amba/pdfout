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

#define MSG(fmt, args...) pdfout_msg ("check info dict: " fmt, ## args)

static int
check_yaml_infodict (yaml_document_t *doc)
{
  yaml_node_t *dict, *key, *value;
  yaml_node_pair_t pair;
  char *name, *string;
  int length, i;

  dict = yaml_document_get_root_node (doc);
  if (dict == NULL)
    {
      MSG ("empty info dict");
      return 0;
    }
  if (dict->type != YAML_MAPPING_NODE)
    {
      MSG ("info dict not a mapping");
      return -1;
    }
  
  length = pdfout_mapping_length (doc, 1);

  if (length == 0)
    {
      MSG ("empty info dict");
      return 0;
    }
  
  for (i = 0; i < length; ++i)
    {
      pair = dict->data.mapping.pairs.start[i];
      key = pdfout_yaml_document_get_node (doc, pair.key);
      value = pdfout_yaml_document_get_node (doc, pair.value);
      assert (key && value);
      if (key->type != YAML_SCALAR_NODE || value->type != YAML_SCALAR_NODE)
	{
	  MSG ("infodict entry not a scalar");
	  return -1;
	}
      name = pdfout_scalar_value (key);
      string = pdfout_scalar_value (value);
      if (strcmp (name, "CreationDate") == 0
	  || strcmp (name, "ModDate") == 0)
	{
	  if (pdfout_check_date_string (string))
	    {
	      MSG ("invalid date string '%s'", string);
	      return -1;
	    }
	}
      else if (strcmp (name, "Trapped") == 0)
	{
	  if (strcmp (string, "True") && strcmp (string, "False") &&
	      strcmp (string, "Unknown"))
	    {
	      MSG ("invalid value '%s' of key 'Trapped'.\n"
		   "valid values are True,False,Unknown", string);
	      return -1;
	    }
	}
      else if (strcmp (name, "Title") && strcmp (name, "Author")
	       && strcmp (name, "Subject") && strcmp (name, "Keywords")
	       && strcmp (name, "Creator") && strcmp (name, "Producer"))
	{
	  MSG ("'%s' is not a valid infodict key. Valid keys are:\n\
Title,Author,Subject,Keywords,Creator,Producer,CreationDate,ModDate,Trapped",
	       name);
	  return -1;
	}
    }
  return 0;
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("update info dict: " fmt, ## args)

int
pdfout_update_info_dict (fz_context *ctx, pdf_document *doc,
			 yaml_document_t *yaml_doc, bool append)
{
  pdf_obj *info = NULL, *info_ref, *new_info;
  yaml_node_pair_t pair;
  yaml_node_t *key, *value, *mapping;
  char *text_string, *key_string, *value_string;
  int length, i, text_string_len;

  if (yaml_doc && check_yaml_infodict (yaml_doc))
    {
      MSG ("invalid info dict");
      return 1;
    }

  info = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Info");
  if (info == NULL)
    {
      info = pdf_new_dict (ctx, doc, 9);
      info_ref = pdf_new_ref (ctx, doc, info);
      pdf_drop_obj (ctx, info);
      pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc), "Info", info_ref);
    }
  else if (append == false)
    {
      new_info = pdf_new_dict (ctx, doc, 9);
      pdf_update_object (ctx, doc, pdf_to_num (ctx, info), new_info);
      pdf_drop_obj (ctx, new_info);
      info = new_info;
    }
  
  if (yaml_doc)
    mapping = yaml_document_get_root_node (yaml_doc);
  
  if (yaml_doc == NULL || mapping == NULL)
    {
      MSG ("removing info dict");
      return 0;
    }
  length = pdfout_mapping_length (yaml_doc, 1);
  if (length == 0)
    {
      MSG ("removing info dict");
      return 0;
    }
  for (i = 0; i < length; ++i)
    {
      pair = mapping->data.mapping.pairs.start[i];
      key = pdfout_yaml_document_get_node (yaml_doc, pair.key);
      key_string = pdfout_scalar_value (key);
      value = pdfout_yaml_document_get_node (yaml_doc, pair.value);
      value_string = pdfout_scalar_value (value);
      if (strcmp (key_string, "ModDate") == 0
	  || strcmp (key_string, "CreationDate") == 0)
	{
	  /* no reencoding needed */
	  pdf_dict_puts_drop (ctx, info, key_string,
			      pdf_new_string (ctx, doc, value_string,
					      strlen (value_string)));
	}
      else if (strcmp (key_string, "Trapped") == 0)
	{
	  /* create name object */
	  pdf_dict_puts_drop (ctx, info, key_string,
			      pdf_new_name (ctx, doc, value_string));
	}
      else
	{
	  /* reencode and create string object */
	  text_string =
	    pdfout_utf8_to_text_string (value_string, strlen (value_string),
					&text_string_len);
	  pdf_dict_puts_drop (ctx, info, key_string,
			      pdf_new_string (ctx, doc, text_string,
					      text_string_len));
	  free (text_string);
	}
    }
  return 0;
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("get info dict: " fmt, ## args)

static int
get_info_dict (yaml_document_t *yaml_doc, fz_context *ctx, pdf_document *doc)
{
  pdf_obj *info;
  int i, mapping, len;
  
  info = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Info");
  len = pdf_dict_len (ctx, info);
  
  if (len <= 0)
    {
      MSG ("empty info dict");
      return 1;
    }

  mapping = pdfout_yaml_document_add_mapping (yaml_doc, NULL, 0);
  for (i = 0; i < len; ++i)
    {
      pdf_obj *key, *val;
      int value_string_len;
      char *name, *value_string;
      
      key = pdf_dict_get_key (ctx, info, i);
      if (pdf_is_name (ctx, key) == 0)
	{
	  MSG ("key not a name object, skipping");
	  continue;
	}
      name = pdf_to_name (ctx, key);
      val = pdf_dict_get_val (ctx, info, i);
      if (strcmp (name, "Title") == 0 || strcmp (name, "Author") == 0
	  || strcmp (name, "Subject") == 0 || strcmp (name, "Keywords") == 0
	  || strcmp (name, "Creator") == 0 || strcmp (name, "Producer") == 0
	  || strcmp (name, "CreationDate") == 0
	  || strcmp (name, "ModDate") == 0)
	{
	  /* value must be string */
	  
	  if (pdf_is_string (ctx, val) == 0)
	    {
	      MSG ("value for key '%s' not a string", name);
	      continue;
	    }

	  value_string =
	    pdfout_text_string_to_utf8 (pdf_to_str_buf (ctx, val),
					pdf_to_str_len (ctx, val),
					&value_string_len);

	  if (strcmp (name, "CreationDate") == 0
	      || strcmp (name, "ModDate") == 0)
	    {
	      if (pdfout_check_date_string (value_string))
		MSG ("broken %s: %s", name, value_string);
	    }

	  pdfout_mapping_push (yaml_doc, mapping, name, value_string);
	  free (value_string);
	}
      else if (strcmp (name, "Trapped") == 0)
	{
	  if (pdf_is_name (ctx, val) == 0)
	    {
	      MSG ("value of key '%s' not a name object, skipping", name);
	      continue;
	    }

	  value_string = pdf_to_name (ctx, val);

	  if (strcmp (value_string, "True") && strcmp (value_string, "False")
	      && strcmp (value_string, "Unknown"))
	    MSG ("illegal value '%s' of key 'Trapped' encountered",
		 value_string);
	  
	  pdfout_mapping_push (yaml_doc, mapping, name, value_string);

	}
    }

  if (pdfout_mapping_length (yaml_doc, mapping) == 0)
    {
      MSG ("empty info dict");
      return 1;
    }
  
  return 0;
}

int
pdfout_get_info_dict (yaml_document_t **yaml_doc_ptr, fz_context *ctx,
		      pdf_document *doc)
{
  yaml_document_t *yaml_doc;

  *yaml_doc_ptr = yaml_doc = XZALLOC (yaml_document_t);
  
  pdfout_yaml_document_initialize (yaml_doc, NULL, NULL, NULL, 1, 1);

  if (get_info_dict (yaml_doc, ctx, doc))
    {
      yaml_document_delete (yaml_doc);
      free (yaml_doc);
      *yaml_doc_ptr = NULL;
      return 1;
    }
    
  return 0;
}
