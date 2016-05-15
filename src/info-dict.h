#ifndef HAVE_PDFOUT_INFO_DICT_H
#define HAVE_PDFOUT_INFO_DICT_H

void pdfout_info_dict_set (fz_context *ctx, pdf_document *doc,
			   pdfout_data *info, bool append);

pdfout_data *pdfout_info_dict_get (fz_context *ctx, pdf_document *doc);

#endif
