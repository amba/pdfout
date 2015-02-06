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

/* libyaml wrappers */

void
pdfout_yaml_parser_initialize (yaml_parser_t *parser)
{
  if (yaml_parser_initialize (parser) == 0)
    error (1, 0, "yaml_parser_initialize");
}

int
pdfout_yaml_parser_load (yaml_parser_t *parser, yaml_document_t *document)
{
  int retval;
  retval = yaml_parser_load (parser, document);

  if (retval == 0)
    pdfout_msg ("load YAML: %s", parser->problem);

  return retval;
}

void
pdfout_yaml_emitter_initialize (yaml_emitter_t *emitter)
{
  if (yaml_emitter_initialize (emitter) == 0)
    error (1, 0, "yaml_emitter_initialize");
  
  yaml_emitter_set_indent (emitter, pdfout_yaml_emitter_indent);
  yaml_emitter_set_width (emitter, pdfout_yaml_emitter_line_width);
  yaml_emitter_set_unicode (emitter, pdfout_yaml_emitter_escape_unicode == 0);
}

void
pdfout_yaml_emitter_open (yaml_emitter_t *emitter)
{
  if (yaml_emitter_open (emitter) == 0)
    error (1, 0, "yaml_emitter_open");
}

void
pdfout_yaml_emitter_dump (yaml_emitter_t *emitter,
			  yaml_document_t *document)
{
  if (yaml_emitter_dump (emitter, document) == 0)
    error (1, 0, "yaml_emitter_dump");
}

void
pdfout_yaml_emitter_close (yaml_emitter_t *emitter)
{
  if (yaml_emitter_close (emitter) == 0)
    error (1, 0, "yaml_emitter_close");
}


void
pdfout_yaml_document_initialize (yaml_document_t *document,
				 yaml_version_directive_t *version_directive,
				 yaml_tag_directive_t *tag_directives_start,
				 yaml_tag_directive_t *tag_directives_end,
				 int start_implicit, int end_implicit)
{
  if (yaml_document_initialize (document, version_directive,
				tag_directives_start, tag_directives_end,
				start_implicit, end_implicit)
      == 0)
    {
      error (1, 0, "yaml_document_initialize");
    }
}

yaml_node_t *
pdfout_yaml_document_get_root_node (yaml_document_t *document)
{
  yaml_node_t *retval = yaml_document_get_root_node (document);
  if (retval == NULL)
    error (1, 0, "yaml_document_get_root_node");
  return retval;
}

yaml_node_t *
pdfout_yaml_document_get_node (yaml_document_t *document, int index)
{
  yaml_node_t *retval = yaml_document_get_node (document, index);
  if (retval == NULL)
    error (1, 0, "yaml_document_get_node: index out of range");
  return retval;
}

int
pdfout_yaml_document_add_scalar (yaml_document_t *document, char *tag,
				 char *value, int length,
				 yaml_scalar_style_t style)
{
  int retval;
  if (length < 0)
    length = strlen (value);
  
  retval = yaml_document_add_scalar (document, (yaml_char_t *) tag,
				     (yaml_char_t *) value, length, style);
  if (retval == 0)
    error (1, 0, "yaml_document_add_scalar");
  return retval;
}

int
pdfout_yaml_document_add_sequence (yaml_document_t *document, char *tag,
				   yaml_sequence_style_t style)
{
  int retval;
  retval = yaml_document_add_sequence (document, (yaml_char_t *) tag, style);
  if (retval == 0)
    error (1, 0, "yaml_document_add_sequence");
  return retval;
}

int
pdfout_yaml_document_add_mapping (yaml_document_t *document, char *tag,
				  yaml_mapping_style_t style)
{
  int retval;
  retval = yaml_document_add_mapping (document, (yaml_char_t *) tag, style);
  if (retval == 0)
    error (1, 0, "yaml_document_add_mapping");
  return retval;
}

int
pdfout_yaml_document_append_sequence_item (yaml_document_t *document,
					   int sequence, int item)
{
  int retval;
  retval = yaml_document_append_sequence_item (document, sequence, item);
  if (retval == 0)
    error (1, 0, "yaml_document_append_sequence_item");
  return retval;
}

int
pdfout_yaml_document_append_mapping_pair (yaml_document_t *document,
					  int mapping, int key, int value)
{
  int retval;
  retval = yaml_document_append_mapping_pair (document, mapping, key, value);
  if (retval == 0)
    error (1, 0, "yaml_document_append_mapping_pair");
  return retval;
}

/* YAML convenience functions */

int
pdfout_load_yaml (yaml_document_t **doc, FILE *file)
{
  int retval;
  yaml_parser_t *parser = XZALLOC (yaml_parser_t);
  
  *doc = XZALLOC (yaml_document_t);
  pdfout_yaml_parser_initialize (parser);
  yaml_parser_set_input_file (parser, file);
  
  retval = pdfout_yaml_parser_load (parser, *doc);
  if (retval == 0)
    {
      yaml_document_delete (*doc);
      free (*doc);
    }

  /* cleanup  */
  yaml_parser_delete (parser);
  free (parser);

  return retval ? 0 : 1;
}

/* default values. can be changed on command line. */
int pdfout_yaml_emitter_indent = 4;
int pdfout_yaml_emitter_line_width = -1;
int pdfout_yaml_emitter_escape_unicode = 0;

void
pdfout_dump_yaml (FILE *file, yaml_document_t *doc)
{
  yaml_emitter_t emitter;
  pdfout_yaml_emitter_initialize (&emitter);
  yaml_emitter_set_output_file (&emitter, file);
  pdfout_yaml_emitter_dump (&emitter, doc);
  yaml_emitter_delete (&emitter);
}

char *
pdfout_scalar_value (yaml_node_t *node)
{
  assert (node && node->type == YAML_SCALAR_NODE);
  return (char *) node->data.scalar.value;
}

void
pdfout_sequence_push (yaml_document_t *doc, int sequence, char *value)
{
  assert (doc);
  assert (sequence);
  int scalar = pdfout_yaml_document_add_scalar (doc, NULL, value, -1,
						YAML_ANY_SCALAR_STYLE);
  pdfout_yaml_document_append_sequence_item (doc, sequence, scalar);
}

int
pdfout_sequence_length (yaml_document_t *doc, int sequence)
{
  assert (doc);
  yaml_node_t *node;
  node = yaml_document_get_node (doc, sequence);
  if (node == NULL || node->type != YAML_SEQUENCE_NODE)
    return 0;
  return node->data.sequence.items.top - node->data.sequence.items.start;
}

int
pdfout_sequence_get (yaml_document_t *doc, int sequence, int index)
{
  yaml_node_t *node;
  node = yaml_document_get_node (doc, sequence);
  assert (node && node->type == YAML_SEQUENCE_NODE);
  assert (index >= 0 && index < pdfout_sequence_length (doc, sequence));
  return node->data.sequence.items.start[index];
}

yaml_node_t *
pdfout_sequence_get_node (yaml_document_t *doc, int sequence, int index)
{
  int item;
  item = pdfout_sequence_get (doc, sequence, index);
  return yaml_document_get_node (doc, item);
}

int
pdfout_mapping_length (yaml_document_t *doc, int mapping)
{
  yaml_node_t *node = yaml_document_get_node (doc, mapping);
  if (node == NULL || node->type != YAML_MAPPING_NODE)
    return 0;
  return node->data.mapping.pairs.top - node->data.mapping.pairs.start;
}

void
pdfout_mapping_push (yaml_document_t *doc, int mapping, char *key, char *value)
{
  int key_id, value_id;
  key_id = pdfout_yaml_document_add_scalar (doc, NULL, key, -1,
					    YAML_ANY_SCALAR_STYLE);
  value_id = pdfout_yaml_document_add_scalar (doc, NULL, value, -1,
					      YAML_ANY_SCALAR_STYLE);
  pdfout_yaml_document_append_mapping_pair (doc, mapping, key_id, value_id);
}

void
pdfout_mapping_push_id (yaml_document_t *doc, int mapping, char *key,
			int value_id)
{
  int key_id;
  key_id =
    pdfout_yaml_document_add_scalar (doc, NULL, key, -1,
				     YAML_ANY_SCALAR_STYLE);
  pdfout_yaml_document_append_mapping_pair (doc, mapping, key_id, value_id);
}

yaml_node_t *
pdfout_mapping_gets_node (yaml_document_t *doc, int mapping,
			  const char *key_string)
{
  int id;
  id = pdfout_mapping_gets (doc, mapping, key_string);
  if (id == 0)
    return NULL;
  return yaml_document_get_node (doc, id);
}

int
pdfout_mapping_gets (yaml_document_t *doc, int mapping, const char *key_string)
{
  yaml_node_t *node, *key;
  yaml_node_pair_t pair;
  int length, i;
  node = yaml_document_get_node (doc, mapping);
  assert (node && node->type == YAML_MAPPING_NODE);
  length = node->data.mapping.pairs.top - node->data.mapping.pairs.start;
  assert (length >= 0);
  for (i = 0; i < length; ++i)
    {
      pair = node->data.mapping.pairs.start[i];
      key = yaml_document_get_node (doc, pair.key);
      if (key && key->type == YAML_SCALAR_NODE
	  && strcasecmp (pdfout_scalar_value (key), key_string) == 0)
	return pair.value;
    }
  return 0;
}

int
pdfout_is_null (const char *string)
{
  return (strcmp (string, "~") == 0 || strcmp (string, "null") == 0
	  || strcmp (string, "Null") == 0 || strcmp (string, "NULL") == 0
	  || strcmp (string, "") == 0);
}

int
pdfout_is_bool (const char *string)
{
  return (strcmp (string, "y") == 0 || strcmp (string, "Y") == 0
	  || strcmp (string, "yes") == 0 || strcmp (string, "Yes") == 0
	  || strcmp (string, "YES") == 0 || strcmp (string, "true") == 0
	  || strcmp (string, "True") == 0 || strcmp (string, "TRUE") == 0
	  || strcmp (string, "on") == 0 || strcmp (string, "On") == 0
	  || strcmp (string, "ON") == 0 || strcmp (string, "n") == 0
	  || strcmp (string, "N") == 0 || strcmp (string, "no") == 0
	  || strcmp (string, "No") == 0 || strcmp (string, "NO") == 0
	  || strcmp (string, "false") == 0 || strcmp (string, "False") == 0
	  || strcmp (string, "FALSE") == 0 || strcmp (string, "off") == 0
	  || strcmp (string, "Off") == 0 || strcmp (string, "OFF") == 0);
}

int
pdfout_is_true (const char *string)
{
  return (strcmp (string, "y") == 0 || strcmp (string, "Y") == 0
	  || strcmp (string, "yes") == 0 || strcmp (string, "Yes") == 0
	  || strcmp (string, "YES") == 0 || strcmp (string, "true") == 0
	  || strcmp (string, "True") == 0 || strcmp (string, "TRUE") == 0
	  || strcmp (string, "on") == 0 || strcmp (string, "On") == 0
	  || strcmp (string, "ON") == 0);
}

int
_pdfout_float_sequence (yaml_document_t *doc, ...)
{
  int sequence;
  char buffer[256];
  va_list ap;
  float value;
  
  va_start (ap, doc);

  sequence =
    pdfout_yaml_document_add_sequence (doc, NULL, YAML_FLOW_SEQUENCE_STYLE);
  
  while ((value = va_arg (ap, double)) != INFINITY)
    {
      pdfout_snprintf (buffer, sizeof buffer, "%g", value);
      pdfout_sequence_push (doc, sequence, buffer);
    }
  va_end (ap);
  return sequence;
}
