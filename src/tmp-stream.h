#ifndef HAVE_PDFOUT_TMP_STREAM_H
#define HAVE_PDFOUT_TMP_STREAM_H

typedef struct pdfout_tmp_stream_s pdfout_tmp_stream;

pdfout_tmp_stream *pdfout_tmp_stream_new (fz_context *ctx);

void pdfout_tmp_stream_drop (fz_context *ctx, pdfout_tmp_stream *stream);

fz_stream *pdfout_tmp_stream_stream (fz_context *ctx,
				    pdfout_tmp_stream *stream);

fz_output *pdfout_tmp_stream_output (fz_context *ctx,
				     pdfout_tmp_stream *stream);

FILE *pdfout_tmpfile(fz_context *ctx);

#endif	/* HAVE_PDFOUT_TMP_STREAM_H */
