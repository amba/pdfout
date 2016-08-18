#include "common.h"

struct pdfout_tmp_stream_s
{
  FILE *tmp;
  fz_stream *stream;
  fz_output *output;
};

pdfout_tmp_stream *
pdfout_tmp_stream_new (fz_context *ctx)
{
  pdfout_tmp_stream *handle = fz_malloc_struct (ctx, pdfout_tmp_stream);
  FILE *tmp = tmpfile();
  if (tmp == NULL)
    pdfout_throw_errno (ctx, "tmpfile failed");
  handle->stream = fz_open_file_ptr (ctx, tmp);
  handle->output = fz_new_output_with_file_ptr (ctx, tmp, false);
  handle->tmp = tmp;

  return handle;
}

fz_stream *
pdfout_tmp_stream_stream (fz_context *ctx, pdfout_tmp_stream *handle)
{
  return handle->stream;
}

fz_output *
pdfout_tmp_stream_output (fz_context *ctx, pdfout_tmp_stream *handle)
{
  return handle->output;
}

void
pdfout_tmp_stream_drop (fz_context *ctx, pdfout_tmp_stream *handle)
{
  fz_drop_output (ctx, handle->output);

  /* Also closes handle->tmp.  */
  fz_drop_stream (ctx, handle->stream);
  free (handle);
}


