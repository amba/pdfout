#ifndef PDFOUT_HAVE_LIBYAML_WRAPPERS_H
#define PDFOUT_HAVE_LIBYAML_WRAPPERS_H

/* FIXME: do not use yaml_documents and cleanup this header.  */

/* FIXME: use incomplete types instead of including the big yaml.h.  */
#include <yaml.h>
/* parser stuff */
void pdfout_yaml_parser_initialize (yaml_parser_t *parser);
/* emitter stuff */
void pdfout_yaml_emitter_initialize (yaml_emitter_t *emitter);
void pdfout_yaml_emitter_close (yaml_emitter_t *emitter);
/* Covenience functions.  */

/* see http://yaml.org/type/null.html */
int pdfout_yaml_is_null (const char *string) _GL_ATTRIBUTE_PURE;

/* see http://yaml.org/type/bool.html */
int pdfout_yaml_is_bool (const char *string) _GL_ATTRIBUTE_PURE;

/* returns 1 for true  */
int pdfout_yaml_is_true (const char *string) _GL_ATTRIBUTE_PURE;

/* Get value of scalar event.  */
char *pdfout_yaml_scalar_value (const yaml_event_t *event) _GL_ATTRIBUTE_PURE;

/* Event types. */
bool pdfout_yaml_is_stream_start (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_stream_end (const yaml_event_t *event) _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_document_start (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_document_end (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_scalar (const yaml_event_t *event) _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_sequence_start (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_sequence_end (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_mapping_start (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;
bool pdfout_yaml_is_mapping_end (const yaml_event_t *event)
  _GL_ATTRIBUTE_PURE;

yaml_event_t *pdfout_yaml_event_new (void);

/* Free an event created with pdfout_yaml_event_new.  */
void pdfout_yaml_event_free (yaml_event_t *event);
  
/* Create and emit an event. Return 0 on success, or 1 on error.  */
/* FIXME: set a problem pointer on error?  */
int pdfout_yaml_document_start_event (yaml_emitter_t *emitter,
				      yaml_event_t *event,
				      yaml_version_directive_t *version,
				      int implicit);
int pdfout_yaml_document_end_event (yaml_emitter_t *emitter,
				    yaml_event_t *event, int implicit);
int pdfout_yaml_scalar_event (yaml_emitter_t *emitter, yaml_event_t *event,
			      const char *value);
int pdfout_yaml_sequence_start_event (yaml_emitter_t *emitter,
				      yaml_event_t *event,
				      yaml_sequence_style_t style);
int pdfout_yaml_sequence_end_event (yaml_emitter_t *emitter,
				    yaml_event_t *event);
int pdfout_yaml_mapping_start_event (yaml_emitter_t *emitter,
				     yaml_event_t *event,
				     yaml_mapping_style_t style);
int pdfout_yaml_mapping_end_event (yaml_emitter_t *emitter,
				   yaml_event_t *event);

/* Emit stream-start event, if EMITTER is not yet open. Return non-zero on
   error.  */
int pdfout_yaml_emitter_open (yaml_emitter_t *emitter, yaml_event_t *event);

/* Delete previous contents of EVENT, then parse next event.  Return non-zero
   on error.  */
int pdfout_yaml_parser_parse (yaml_parser_t *parser, yaml_event_t *event);
			      
			      
/* Parse stream/document start events.
   Return values:
   0: Parsed a document start event.
   1: Empty YAML stream.
   2: Error. errno and parser->problem describe the problem.  */
int pdfout_yaml_parser_start (yaml_parser_t *parser, yaml_event_t *event);







/* FIXME: Legacy stuff, remove */

void pdfout_yaml_emitter_dump (yaml_emitter_t *emitter, yaml_document_t *doc);
int pdfout_yaml_parser_load (yaml_parser_t *parser, yaml_document_t *document);
void
pdfout_yaml_document_initialize (yaml_document_t *document,
				 yaml_version_directive_t *version_directive,
				 yaml_tag_directive_t *tag_directives_start,
				 yaml_tag_directive_t *tag_directives_end,
				 int start_implicit, int end_implicit);
yaml_node_t *pdfout_yaml_document_get_node (yaml_document_t *document,
					    int index);
/* Libyaml will use malloc to duplicate the
   buffer. If length is -1, libyaml will use strlen (value).  */
int pdfout_yaml_document_add_scalar (yaml_document_t *document,
				     char *tag, const char *value, int length,
				     yaml_scalar_style_t style);

int pdfout_yaml_document_add_sequence (yaml_document_t *document,
				       char *tag, yaml_sequence_style_t style);

int pdfout_yaml_document_add_mapping (yaml_document_t *document,
				      char *tag, yaml_mapping_style_t style);

int pdfout_yaml_document_append_sequence_item (yaml_document_t *document,
					       int sequence, int item);

int pdfout_yaml_document_append_mapping_pair (yaml_document_t *document,
					      int mapping, int key, int value);

#endif	/* !PDFOUT_HAVE_LIBYAML_WRAPPERS_H */
