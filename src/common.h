#ifndef PDFOUT_COMMON_H
#define PDFOUT_COMMON_H

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <argp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <sysexits.h>
#include <math.h>
#include <stdbool.h>
#include <error.h>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

#include <yaml.h>

#include <uniconv.h>

#include <xalloc.h>
#include <xvasprintf.h>
#include <argmatch.h>

#include "shared-with-tests.h"

/* global variables */

extern yaml_document_t *pdfout_default_dest_array;

extern int pdfout_yaml_emitter_indent;
extern int pdfout_yaml_emitter_line_width;
extern int pdfout_yaml_emitter_escape_unicode;

enum pdfout_text_string_mode {
  PDFOUT_TEXT_STRING_DEFAULT,
  PDFOUT_TEXT_STRING_UTF16,
  PDFOUT_TEXT_STRING_PDFDOC_QUESTION_MARK,
  PDFOUT_TEXT_STRING_PDFDOC_ERROR
};

extern enum pdfout_text_string_mode
pdfout_text_string_mode;
 
enum {
  PDFOUT_TXT_YAML_LINES = 0,
  PDFOUT_TXT_YAML_SPANS,
  PDFOUT_TXT_YAML_CHARACTERS,
};

/* silently replaces broken multibytes with a '?' character */
char *pdfout_text_string_to_utf8 (char *inbuf, int inbuf_len, int *outbuf_len);

/* dies on all errors, so make sure that the UTF-8 is valid */
char *pdfout_utf8_to_text_string (char *inbuf, int inbuf_len, int *outbuf_len);

void pdfout_print_yaml_page (fz_context *ctx, pdf_document *pdf_doc,
			     int page_number, yaml_emitter_t *emitter,
			     int mode);

void pdfout_print_txt_page (fz_context *ctx, pdf_document *doc,
			    int page_number, FILE *out);

/* on error, returns non-zero and *DOC is undefined */
int pdfout_outline_load (yaml_document_t **doc, FILE *input,
			 enum pdfout_outline_format format);

/* on error, returns non-zero  */
int pdfout_outline_dump (FILE *output, yaml_document_t *doc,
			 enum pdfout_outline_format format);

/* remove outline, if YAML_DOC is NULL or empty.
   return values:
   0: success
   1: invalid yaml_doc
   2: no document catalog */
int pdfout_outline_set (fz_context *ctx, pdf_document *doc,
			yaml_document_t *yaml_doc);

/* return values:
   0: *YAML_DOC will point to the allocated outline, which has to be freed by
   the user.
   1: no outline, *YAML_DOC is set to NULL  */
int pdfout_outline_get (yaml_document_t **yaml_doc, fz_context *ctx,
			pdf_document *pdf_doc);


/* remove page labels if YAML_DOC is NULL or empty  */
int pdfout_update_page_labels (fz_context *ctx, pdf_document *doc,
			       yaml_document_t *yaml_doc);

/* remove all keys if YAML_DOC is NULL or empty.
   set APPEND to keep the existing entries */
int pdfout_update_info_dict (fz_context *ctx, pdf_document *doc,
			     yaml_document_t *yaml_doc, bool append);

/* return values:
   0: *YAML_DOC will point to allocated info dict, a list of scalar
   key-value pairs.
   1: empty info dict and *YAML_DOC is set to NULL */
int pdfout_get_info_dict (yaml_document_t **yaml_doc, fz_context *ctx,
			  pdf_document *doc);
/* see 7.9.4 Dates in pdf-1.7 */
int pdfout_check_date_string (const char *date);

/* returns values:
   0: *YAML_DOC will point to allocated page labels
   1: no page labelsm *YAML_DOC is set to NULL  */
int pdfout_get_page_labels (yaml_document_t **yaml_doc, fz_context *ctx,
			    pdf_document *doc);

/* makes incremental update if OUTPUT_FILENAME is NULL
   calls exit (1) on error  */
void pdfout_write_document (fz_context *ctx, pdf_document *doc,
			    char *pdf_filename, char *output_filename);

/* calls exit (1) on error */
pdf_document *pdfout_pdf_open_document (fz_context *ctx, char *file);

/* sets *endptr to nptr on overflow */
int pdfout_strtoint (const char *nptr, char **endptr);

/* dies on all errors */
int pdfout_strtoint_null (const char *string);

/* returns NaN on all errors */
float pdfout_strtof_nan (const char *string);

/* dies on all errors */
float pdfout_strtof (const char *string);

/* dies if result would be truncated */
int pdfout_snprintf (char *str, int size, const char *fmt, ...)
  __printflike (3,4);


/* libyaml wrappers  */

void
pdfout_yaml_document_initialize (yaml_document_t *document,
				 yaml_version_directive_t *version_directive,
				 yaml_tag_directive_t *tag_directives_start,
				 yaml_tag_directive_t *tag_directives_end,
				 int start_implicit, int end_implicit);

/* parser stuff */
void pdfout_yaml_parser_initialize (yaml_parser_t *parser);

int pdfout_yaml_parser_load (yaml_parser_t *parser, yaml_document_t *document);

/* emitter stuff */
void pdfout_yaml_emitter_initialize (yaml_emitter_t *emitter);

void pdfout_yaml_emitter_open (yaml_emitter_t *emitter);

void pdfout_yaml_emitter_dump (yaml_emitter_t *emitter,
			       yaml_document_t *document);

void pdfout_yaml_emitter_close (yaml_emitter_t *emitter);

/* node stuff */

/* FIXME! pdfout_yaml_document_get_root_node: throws exception if document is
   empty */
yaml_node_t *pdfout_yaml_document_get_root_node (yaml_document_t *document);

/* pdfout_yaml_document_get_node: throws exception if index is out of range */
yaml_node_t *pdfout_yaml_document_get_node (yaml_document_t *document,
					    int index);
/*
   pdfout_yaml_document_add_scalar: libyaml will use malloc to duplicate the
   buffer. if length is -1, libyaml will use strlen (value).
*/
int pdfout_yaml_document_add_scalar (yaml_document_t *document,
        char *tag, char *value, int length,
        yaml_scalar_style_t style);

int pdfout_yaml_document_add_sequence (yaml_document_t *document,
        char *tag, yaml_sequence_style_t style);

int pdfout_yaml_document_add_mapping (yaml_document_t *document,
				      char *tag, yaml_mapping_style_t style);

int pdfout_yaml_document_append_sequence_item (yaml_document_t *document,
					       int sequence, int item);

int pdfout_yaml_document_append_mapping_pair (yaml_document_t *document,
					      int mapping, int key, int value);

/* YAML convenience functions  */

/* on error, prints message describing the problem and returns non-zero.  */
int pdfout_load_yaml (yaml_document_t **doc, FILE *file);

void pdfout_dump_yaml (FILE *file, yaml_document_t *doc);

char *pdfout_scalar_value (yaml_node_t *scalar_node);

/* search is case-insensitive */
int pdfout_mapping_gets (yaml_document_t *doc, int mapping,
			 const char *key_string);

yaml_node_t *pdfout_mapping_gets_node (yaml_document_t *doc, int mapping,
				      const char *key_string);

void pdfout_sequence_push (yaml_document_t *doc, int sequence,
			  char *value);

int pdfout_sequence_length (yaml_document_t *doc, int sequence);

int pdfout_sequence_get (yaml_document_t *doc, int sequence, int index);

yaml_node_t *pdfout_sequence_get_node (yaml_document_t *doc, int sequence,
				       int index);

int pdfout_mapping_length (yaml_document_t *doc, int mapping);

void pdfout_mapping_push (yaml_document_t *doc, int mapping,
				       char *key, char *value);

void pdfout_mapping_push_id (yaml_document_t *doc, int mapping,
					  char *key, int value_id);
/* see http://yaml.org/type/null.html */
int pdfout_is_null (const char *string);

/* see http://yaml.org/type/bool.html */
int pdfout_is_bool (const char *string);

/* returns 1 for true  */
int pdfout_is_true (const char *string);

/* last value must be INFINITY */
int _pdfout_float_sequence (yaml_document_t *doc, ...);

#define pdfout_float_sequence(doc, floats...) \
  _pdfout_float_sequence (doc, ## floats, INFINITY)

#endif	/* !PDFOUT_COMMON_H */

