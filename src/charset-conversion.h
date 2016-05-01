#ifndef HAVE_CHARSET_CONVERSION_H
#define HAVE_CHARSET_CONVERSION_H

char *
pdfout_pdf_to_utf8 (fz_context *ctx, const char *inbuf, int inbuf_len,
		    int *outbuf_len);

char *
pdfout_utf8_to_pdf (fz_context *ctx, const char *inbuf, int inbuf_len,
		    int *outbuf_len);

/* If a codepoint cannot be stored in the target encoding, throw
   FZ_ERROR_ABORT. On all other errors throw FZ_ERROR_GENERIC.  */
void
pdfout_char_conv_buffer (fz_context *ctx, const char *fromcode,
			 const char *tocode, const char *src, int srclen,
			 fz_buffer *buf);

/* Return newly allocated buffer and store it's length in *LENGTHP.  */
char *
pdfout_char_conv (fz_context *ctx, const char *fromcode, const char *tocode,
		  const char *src, int srclen, int *lengthp);

#endif /* !HAVE_CHARSET_CONVERSION_H */
