#ifndef HAVE_PDFOUT_PAGE_LABELS_H
#define HAVE_PDFOUT_PAGE_LABELS_H

void pdfout_page_labels_set (fz_context *ctx, pdf_document *doc,
			     pdfout_data *labels);

pdfout_data *pdfout_page_labels_get (fz_context *ctx, pdf_document *doc);

#endif	/* ! HAVE_PDFOUT_PAGE_LABELS_H */
