#ifndef HAVE_PDFOUT_PAGE_LABELS_H
#define HAVE_PDFOUT_PAGE_LABELS_H

typedef enum pdfout_page_labels_style_e {
  PDFOUT_PAGE_LABELS_NO_NUMBERING,
  PDFOUT_PAGE_LABELS_ARABIC,
  PDFOUT_PAGE_LABELS_UPPER_ROMAN,
  PDFOUT_PAGE_LABELS_LOWER_ROMAN,
  PDFOUT_PAGE_LABELS_UPPER,
  PDFOUT_PAGE_LABELS_LOWER
}  pdfout_page_labels_style_t;

/* One element of a page labels array.  */
typedef struct pdfout_page_labels_mapping_s {
  int page;	       /*  page index. Must be 0 for the first member.  */
  pdfout_page_labels_style_t style;
  char *prefix;	       /* Can be NULL.  */
  int start;	       /* If 0, use default value of 1.  */
} pdfout_page_labels_mapping_t;

typedef struct pdfout_page_labels_s  pdfout_page_labels_t;

/* Allocate new pdfout_page_labels_t.  */
pdfout_page_labels_t *pdfout_page_labels_new (void);

/* Free page labels allocated with pdfout_page_labels_new.  */
void pdfout_page_labels_free (pdfout_page_labels_t *labels);

/* Return 0 on success, or 1 on error.  */
int pdfout_page_labels_push (pdfout_page_labels_t *labels,
			     const pdfout_page_labels_mapping_t *mapping);

/* Return number of elements in LABELS, which must be non-null.  */
size_t pdfout_page_labels_length (const pdfout_page_labels_t *labels)
  _GL_ATTRIBUTE_PURE;

/* Return INDEX'th element of a page labels array.  */
pdfout_page_labels_mapping_t *
pdfout_page_labels_get_mapping (const pdfout_page_labels_t *labels,
				size_t index) _GL_ATTRIBUTE_PURE;
							      

/* LABELS must be well-formed.
   Remove page labels if LABELS is NULL.  */
int pdfout_page_labels_set (fz_context *ctx, pdf_document *doc,
			    const pdfout_page_labels_t *labels);


/* Return 0, if all page numbers are < PAGE_COUNT.  */
int pdfout_page_labels_check (const pdfout_page_labels_t *labels,
			      int page_count);

/* Returns values:
   0: page labels are valid, *LABELS will point to allocated page labels if
   non-empty, otherwise *LABELS is set to NULL.
   1: part of the page labels were broken, *LABELS will point, to the valid
   ones.
   Always, if *LABELS is not NULL, pdfout_page_labels_length (*labels) will be
   >= 1.  */
int pdfout_page_labels_get (pdfout_page_labels_t **labels,
			    fz_context *ctx, pdf_document *doc);

/* LABELS must be non-null and well-formed. Return 0 on success,
   or 1 on error.  */
int pdfout_page_labels_to_yaml (yaml_emitter_t *emitter,
				const pdfout_page_labels_t *labels);

/* Return values:
   0: *LABELS points to newly allocated page labels. If YAML stream is empty,
   *LABELS is set to NULL.
   1: Error: *LABELS is undefined.  */
int pdfout_page_labels_from_yaml (pdfout_page_labels_t **labels,
				  yaml_parser_t *parser);

/* Returns newly allocated string. Length of LABELS must be >= 1.
   If RESULTBUF is not NULL and result, including null-byte, fits into lengthp
   bytes, it is put into RESULTBUF, RESULTBUF is returned, and *LENGTHP is
   unchanged.  Otherwise, a freshly allocated string is returned and its length
   , including the terminating null-byte, is returned in *LENGTHP.  */
char *pdfout_page_labels_print (int page_index,
				const pdfout_page_labels_t *labels,
				char *resultbuf, size_t *lengthp);

#endif	/* ! HAVE_PDFOUT_PAGE_LABELS_H */
