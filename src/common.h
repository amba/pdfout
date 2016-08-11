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

#include <xalloc.h>
#include <xvasprintf.h>
#include <argmatch.h>

#include "shared-with-tests.h"
#include "libyaml-wrappers.h"
#include "data.h"
#include "charset-conversion.h"
#include "page-labels.h"
#include "info-dict.h"

/* global variables */

extern yaml_document_t *pdfout_default_dest_array;
 
enum {
  PDFOUT_TXT_YAML_LINES = 0,
  PDFOUT_TXT_YAML_SPANS,
  PDFOUT_TXT_YAML_CHARACTERS,
};

pdf_obj *pdfout_resolve_dest(fz_context *ctx, pdf_document *doc, pdf_obj *dest,
			     fz_link_kind kind);


#define pdfout_x2nrealloc(ctx, p, pn, t) \
  ((t *) pdfout_x2nrealloc_imp (ctx, p, pn, sizeof (t)))

void *pdfout_x2nrealloc_imp (fz_context *ctx, void *p, int *pn, unsigned s);
  
void pdfout_print_yaml_page (fz_context *ctx, pdf_document *pdf_doc,
			     int page_number, yaml_emitter_t *emitter,
			     int mode);

void pdfout_text_get_page (FILE *stream, fz_context *ctx,
			   pdf_document *doc, int page_number);

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

/* returns values:
   0: *YAML_DOC will point to allocated page labels
   1: no page labelsm *YAML_DOC is set to NULL  */
int pdfout_get_page_labels (yaml_document_t **yaml_doc, fz_context *ctx,
			    pdf_document *doc);

/* makes incremental update if OUTPUT_FILENAME is NULL
   calls exit (1) on error  */
void pdfout_write_document (fz_context *ctx, pdf_document *doc,
			    const char *pdf_filename,
			    const char *output_filename);

/* calls exit (1) on error */
pdf_document *pdfout_pdf_open_document (fz_context *ctx, const char *file);

char *pdfout_check_utf8 (const char *s, size_t n);
  
/* On error, return -1.  */
int pdfout_strtoui (fz_context *ctx, const char *string);

/* sets *endptr to nptr on overflow */
int pdfout_strtoint (fz_context *ctx, const char *nptr, char **endptr);

int pdfout_strtoint_old (const char *nptr, char **endptr);

/* dies on all errors */
int pdfout_strtoint_null (fz_context *ctx, const char *string);

int pdfout_strtoint_null_old (const char *string);

/* returns NaN on all errors */
float pdfout_strtof_nan (const char *string);

/* dies on all errors */
float pdfout_strtof (fz_context *ctx, const char *string);
float pdfout_strtof_old (const char *string);

void PDFOUT_NORETURN PDFOUT_PRINTFLIKE (2)
pdfout_throw_errno (fz_context *ctx, const char *fmt, ...);

PDFOUT_PRINTFLIKE (2)
void pdfout_warn (fz_context *ctx, const char *fmt, ...);

void PDFOUT_NORETURN
pdfout_vthrow (fz_context *ctx, const char *fmt, va_list ap);

void PDFOUT_NORETURN PDFOUT_PRINTFLIKE (2)
pdfout_throw (fz_context *ctx, const char *fmt, ...);


int PDFOUT_PRINTFLIKE (4)
pdfout_snprintf_imp (fz_context *ctx, char *buf, int size,
		     const char *fmt, ...);

#define pdfout_snprintf(ctx, buff, fmt, args...)		\
  pdfout_snprintf_imp (ctx, buff, sizeof buff, fmt, ## args)


/* dies if result would be truncated */
int pdfout_snprintf_old (char *str, size_t size, const char *fmt, ...)
  PDFOUT_PRINTFLIKE (3);

/* buff must be of type char[N].  */
#define PDFOUT_SNPRINTF_OLD(buff, fmt, args...)		\
  pdfout_snprintf_old (buff, sizeof buff, fmt, ## args)

#define PDFOUT_X2NREALLOC(p, pn, t) ((t *) x2nrealloc (p, pn, sizeof (t)))

/* FIXME! remove/rename following cruft:  */

/* on error, prints message describing the problem and returns non-zero.  */
int pdfout_load_yaml (yaml_document_t **doc, FILE *file);

void pdfout_dump_yaml (FILE *file, yaml_document_t *doc);

char *pdfout_scalar_value (yaml_node_t *scalar_node) _GL_ATTRIBUTE_PURE;

int pdfout_mapping_gets (yaml_document_t *doc, int mapping,
			 const char *key_string);

yaml_node_t *pdfout_mapping_gets_node (yaml_document_t *doc, int mapping,
				      const char *key_string);

void pdfout_sequence_push (yaml_document_t *doc, int sequence,
			   const char *value);

int pdfout_sequence_length (yaml_document_t *doc, int sequence);

int pdfout_sequence_get (yaml_document_t *doc, int sequence, int index);

yaml_node_t *pdfout_sequence_get_node (yaml_document_t *doc, int sequence,
				       int index);

int pdfout_mapping_length (yaml_document_t *doc, int mapping);

void pdfout_mapping_push (yaml_document_t *doc, int mapping,
			  const char *key, const char *value);

void pdfout_mapping_push_id (yaml_document_t *doc, int mapping,
			     const char *key, int value_id);

#define pdfout_is_null pdfout_yaml_is_null
#define pdfout_is_bool pdfout_yaml_is_bool
#define pdfout_is_true pdfout_yaml_is_true

/* last value must be INFINITY */
/* FIXME: operate on rectangles?  */
int _pdfout_float_sequence (yaml_document_t *doc, ...);

#define pdfout_float_sequence(doc, floats...) \
  _pdfout_float_sequence (doc, ## floats, INFINITY)

#endif	/* !PDFOUT_COMMON_H */

