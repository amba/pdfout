#include "common.h"

int
pdfout_strtoint (fz_context *ctx, const char *nptr, char **endptr)
{
  long rv = strtol (nptr, endptr, 10);
  if (rv > INT_MAX || rv < INT_MIN)
    pdfout_throw (ctx, "int overflow with argument '%s'", nptr);
  return rv;
}

int
pdfout_strtoint_null (fz_context *ctx, const char *string)
{
  int result;
  char *endptr;
  result = pdfout_strtoint (ctx, string, &endptr);
  if (endptr == string || endptr[0] != '\0')
    pdfout_throw (ctx, "pdfout_strtoint_null: invalid int: '%s'", string);
  return result;
}


float
pdfout_strtof (fz_context *ctx, const char *string)
{
  float result;
  char *endptr;

  errno = 0;

  result = strtof (string, &endptr);

  if (errno || endptr == string || endptr[0] != '\0')
    pdfout_throw_errno (ctx, "pdfout_strtof: invalid float: '%s'", string);
  
  return result;
}

void
pdfout_vthrow (fz_context *ctx, const char *fmt, va_list ap)
{
  /* fz_throw does not use a C99 printf internally, so format here.  */
  char buffer[4096];
  pdfout_vsnprintf (ctx, buffer, fmt, ap);
  fz_throw (ctx, FZ_ERROR_GENERIC, "%s", buffer);
}

void
pdfout_throw (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  pdfout_vthrow (ctx, fmt, ap);
}

void
pdfout_warn (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  /* fz_warn does not use a C99 printf internally, so format here.  */
  char buffer[4096];
  pdfout_vsnprintf (ctx, buffer, fmt, ap);
  fz_warn (ctx, "%s", buffer);
}
  
void
pdfout_throw_errno (fz_context *ctx, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  
  if (errno)
    {
      char buf[2048];
      pdfout_snprintf (ctx, buf, "%s: %s", fmt, strerror (errno));
      pdfout_vthrow (ctx, buf, ap);
    }
  
  pdfout_vthrow (ctx, fmt, ap);
}

int
pdfout_snprintf_imp (fz_context *ctx, char *buf, int size,
		     const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  return pdfout_vsnprintf_imp (ctx, buf, size, fmt, ap);
}

int
pdfout_vsnprintf_imp (fz_context *ctx, char *buf, int size,
		      const char *fmt, va_list ap)
{
  errno = 0;
  int ret = vsnprintf (buf, size, fmt, ap);
  if (ret < 0)
    pdfout_throw_errno (ctx, "output error in pdfout_snprintf");
  if (ret >= size)
    pdfout_throw (ctx, "string truncated in pdfout_snprintf");
  return ret;
}

static void zero_term_buffer (fz_context *ctx, fz_buffer *buf)
{
  fz_write_buffer_byte (ctx, buf, 0);
  --buf->len;
}

ssize_t
pdfout_getline (fz_context *ctx, fz_buffer **buffer_ptr, fz_stream *stm)
{
  if (*buffer_ptr == NULL)
    *buffer_ptr = fz_new_buffer(ctx, 0);

  fz_buffer *buffer = *buffer_ptr;
  
  buffer->len = 0;
  ssize_t len = 0;
  while (1)
    {
      int c = fz_read_byte (ctx, stm);
      if (c == EOF)
	{
	  if (len == 0)
	    return -1;
	  zero_term_buffer(ctx, buffer);
	  return len;
	}
      fz_write_buffer_byte(ctx, buffer, c);
      ++len;
      if (c == '\n')
	{
	  zero_term_buffer (ctx, buffer);
	  return len;
	}
    }
}
