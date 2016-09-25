#include "common.h"

void
pdfout_copy_stream (fz_context *ctx, fz_stream *from, fz_output *too)
{
  /* Do not use fz_read_all, as it has a size limitation.  */
  int buffer_len = 32768;
  unsigned char *buffer = fz_malloc (ctx, buffer_len);

  int read;
  fz_seek (ctx, from, 0, SEEK_SET);
  do
    {
      read = fz_read (ctx, from, buffer, buffer_len);
      fz_write (ctx, too, buffer, read);
    }
  while (read == buffer_len);

  free (buffer);
}

void
pdfout_write_document (fz_context *ctx, pdf_document *doc,
		       const char *pdf_filename, const char *output_filename)
{
  pdf_write_options opts = { 0 };
  
  if (output_filename == NULL)
    {
      /* Incremental update.  */
      opts.do_incremental = 1;
      return pdf_save_document (ctx, doc, pdf_filename, &opts);
    }

  /* Write new pdf to a temporary file, then copy it to the destination.
     Do this to avoid clobbering the original file.  */

  
  pdfout_tmp_stream *tmp = pdfout_tmp_stream_new (ctx);
  fz_output *tmp_out = pdfout_tmp_stream_output (ctx, tmp);
  pdf_write_document (ctx, doc, tmp_out, &opts);

  fz_stream *input = pdfout_tmp_stream_stream (ctx, tmp);
  fz_output *output = fz_new_output_with_path (ctx, output_filename, false);

  pdfout_copy_stream (ctx, input, output);
  fz_drop_output (ctx, output);
  pdfout_tmp_stream_drop (ctx, tmp);
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
