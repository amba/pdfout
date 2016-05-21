#ifndef HAVE_PDFOUT_PAGE_LABELS_H
#define HAVE_PDFOUT_PAGE_LABELS_H

/* LABELS must be well-formed.
   Remove page labels if LABELS is NULL.  */
void pdfout_page_labels_set (fz_context *ctx, pdf_document *doc,
			     pdfout_data *labels);


pdfout_data * pdfout_page_labels_get (pdfout_page_labels_t **labels,
				      fz_context *ctx, pdf_document *doc);


/* Returns newly allocated string. Length of LABELS must be >= 1.  */
char *pdfout_page_labels_print (int page_index, pdfout_data *labels);

#endif	/* ! HAVE_PDFOUT_PAGE_LABELS_H */
