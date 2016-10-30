#ifndef PDFOUT_COMMON_H
#define PDFOUT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>

#include <getopt.h>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>

#include "tmp-stream.h"
#include "data.h"
#include "charset-conversion.h"
#include "page-labels.h"
#include "info-dict.h"
#include "outline.h"

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define PDFOUT_PRINTFLIKE(index)			\
  __attribute__ ((format (printf, index, index + 1)))
# define PDFOUT_WUR __attribute__ ((warn_unused_result))
# define PDFOUT_UNUSED __attribute__ ((__unused__))
# define PDFOUT_NORETURN __attribute__ ((__noreturn__))
#else
# define PDFOUT_PRINTFLIKE(index) /* empty */
# define PDFOUT_WUR
# define PDFOUT_UNUSED
# define PDFOUT_NORETURN
#endif

static inline bool
pdfout_isspace (char c)
{
  return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t'
    || c == '\v';
}

static inline bool
pdfout_isdigit (char c)
{
  return c >= '0' && c <= '9';
}

static inline bool
pdfout_isxdigit (char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
    || (c >= 'A' && c <= 'F');
}

static inline char
pdfout_tolower (char c)
{
  if (c >= 'A' && c <= 'Z')
    return c + ('a' - 'A');
  else
    return c;
}

static inline char
pdfout_toupper (char c)
{
  if (c >= 'a' && c <= 'z')
    return c - ('a' - 'A');
  else
    return c;
}


void pdfout_copy_stream (fz_context *ctx, fz_stream *from, fz_output *too);

pdf_document *
pdfout_create_blank_pdf (fz_context *ctx, int page_count, fz_rect *rect);

pdf_obj *pdfout_resolve_dest(fz_context *ctx, pdf_document *doc, pdf_obj *dest,
			     fz_link_kind kind);


#define pdfout_x2nrealloc(ctx, p, pn, t) \
  ((t *) pdfout_x2nrealloc_imp (ctx, p, pn, sizeof (t)))

void *pdfout_x2nrealloc_imp (fz_context *ctx, void *p, int *pn, unsigned s);

void pdfout_text_get_page (FILE *stream, fz_context *ctx,
			   pdf_document *doc, int page_number);


/* makes incremental update if OUTPUT_FILENAME is NULL, throw on error  */
void pdfout_write_document (fz_context *ctx, pdf_document *doc,
			    const char *pdf_filename,
			    const char *output_filename);

int pdfout_uctomb (fz_context *ctx, uint8_t *buf, uint32_t uc, int n);

char *pdfout_check_utf8 (const char *s, size_t n);
  
/* sets *endptr to nptr on overflow */
int pdfout_strtoint (fz_context *ctx, const char *nptr, char **endptr);

int pdfout_strtoint_old (const char *nptr, char **endptr);

/* dies on all errors */
int pdfout_strtoint_null (fz_context *ctx, const char *string);

/* returns NaN on all errors */
float pdfout_strtof_nan (const char *string);

/* dies on all errors */
float pdfout_strtof (fz_context *ctx, const char *string);
float pdfout_strtof_old (const char *string);

ssize_t
pdfout_getline (fz_context *ctx, fz_buffer **buffer, fz_stream *stm);

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

int pdfout_vsnprintf_imp (fz_context *ctx, char *buf, int size, const char *fmt,
			  va_list ap);


#define pdfout_snprintf(ctx, buff, fmt, args...)		\
  pdfout_snprintf_imp (ctx, buff, sizeof buff, fmt, ## args)

#define pdfout_vsnprintf(ctx, buff, fmt, ap)		\
  pdfout_vsnprintf_imp (ctx, buff, sizeof buff, fmt, ap)


#endif	/* !PDFOUT_COMMON_H */

