#ifndef HAVE_PDFOUT_DEBUG_IO_HANDLE_H
#define HAVE_PDFOUT_DEBUG_IO_HANDLE_H

typedef struct debug_io_handle_s debug_io_handle;

debug_io_handle *debug_io_handle_new (fz_context *ctx);

void debug_io_handle_drop (fz_context *ctx, debug_io_handle *handle);

fz_stream *debug_io_handle_stream (fz_context *ctx, debug_io_handle *handle);

fz_output *debug_io_handle_output (fz_context *ctx, debug_io_handle *handle);


#endif	/* HAVE_PDFOUT_DEBUG_IO_HANDLE_H */
