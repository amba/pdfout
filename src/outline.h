#ifndef HAVE_PDFOUT_OUTLINE_H
#define HAVE_PDFOUT_OUTLINE_H

void pdfout_outline_set (fz_context *ctx, pdf_document *doc,
			 pdfout_data *outline);

pdfout_data *pdfout_outline_get (fz_context *ctx, pdf_document *doc);

#endif
