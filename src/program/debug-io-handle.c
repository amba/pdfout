#include "shared.h"

struct debug_io_handle_s
{
  FILE *tmp;
  fz_stream *stream;
  fz_output *output;
};

debug_io_handle *
debug_io_handle_new (fz_context *ctx)
{
  debug_io_handle *handle = fz_malloc_struct (ctx, debug_io_handle);
  FILE *tmp = tmpfile();
  if (tmp == NULL)
    pdfout_throw_errno (ctx, "tmpfile failed");
  handle->stream = fz_open_file_ptr (ctx, tmp);
  handle->output = fz_new_output_with_file_ptr (ctx, tmp, false);
  handle->tmp = tmp;

  return handle;
}
fz_stream *
debug_io_handle_stream (fz_context *ctx, debug_io_handle *handle)
{
  return handle->stream;
}

fz_output *
debug_io_handle_output (fz_context *ctx, debug_io_handle *handle)
{
  return handle->output;
}

void
debug_io_handle_drop (fz_context *ctx, debug_io_handle *handle)
{
  fz_drop_output (ctx, handle->output);

  /* Also closes handle->tmp.  */
  fz_drop_stream (ctx, handle->stream);
}


