#include "common.h"

void
pdfout_write_document (fz_context *ctx, pdf_document *doc,
		       const char *pdf_filename, const char *output_filename)
{
  pdf_write_options opts = { 0 };
  char *write_filename; 
  bool use_tmp = false;
  
  if (output_filename == NULL)
    {
      /* Incremental update.  */
      opts.do_incremental = 1;
      return pdf_save_document (ctx, doc, pdf_filename, &opts);
    }

  /* Write new pdf to a temporary file, then copy it to the destination.
     Do this to avoid clobbering the original file.  */

  FILE *tmp = tmpfile ();

  /* Closing the output will not close the FILE *.  */
  fz_output *out = fz_new_output_with_file_ptr (ctx, tmp, false);

  pdf_write_document (ctx, doc, out, &opts);
  fz_drop_output (ctx, out);

  
}

pdf_document *
pdfout_create_blank_pdf (fz_context *ctx, int page_count, fz_rect *rect)
{
  fz_rect default_rect = {0, 0, 595.2756f, 841.8898f};
  if (rect == NULL)
    rect = &default_rect;
  
  pdf_document *doc = pdf_create_document (ctx);
  
  pdf_obj *resources = pdf_new_dict (ctx, doc, 0);
  fz_buffer *contents = fz_new_buffer (ctx, 0);

  for (int i = 0; i < page_count; ++i)
    {
      pdf_obj *page = pdf_add_page (ctx, doc, rect, 0, resources, contents);
      pdf_insert_page (ctx, doc, i, page);
    }

  return doc;
}
