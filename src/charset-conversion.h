#ifndef HAVE_CHARSET_CONVERSION_H
#define HAVE_CHARSET_CONVERSION_H

char *
pdfout_pdf_to_utf8 (fz_context *ctx, const char *inbuf, size_t inbuf_len,
		    size_t *outbuf_len);

char *
pdfout_utf8_to_pdf (fz_context *ctx, const char *inbuf, size_t inbuf_len,
		    size_t *outbuf_len);
  
/* If a codepoint can not be stored in the target encoding, throw
   FZ_ERROR_ABORT. On all other errors throw FZ_ERROR_GENERIC.  */
char *
pdfout_char_conv (fz_context *ctx, const char *fromcode, const char *tocode,
		     const char *src, size_t srclen,
		     char *resultbuf, size_t *lengthp);

/* char * */
/* pdfout_check_enc (fz_context *ctx, const char *code, const char *src, */
/* 		  size_t srclen); */
/* char * */
/* pdfout_u8_check (fz_context *ctx, const char *src, size_t len); */
#endif /* !HAVE_CHARSET_CONVERSION_H */
