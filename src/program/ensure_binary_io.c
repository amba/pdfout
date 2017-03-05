#include "shared.h"

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>

static void
safe_setmode (fz_context *ctx, FILE *stream, int mode)
{
  int fd = fileno (stream);
  if (_setmode (fd, mode) == -1)
    pdfout_throw_errno (ctx, "cannot set fd %d to mode %d", fd, mode);
}

void
pdfout_ensure_binary_io (fz_context *ctx)
{
  safe_setmode (ctx, stdout, O_BINARY);
  safe_setmode (ctx, stdin, O_BINARY);
  safe_setmode (ctx, stderr, O_BINARY);
  
}

#else  /* !_WIN32, i.e. Unix */

void
pdfout_ensure_binary_io (fz_context *ctx)
{
  /* Do nothing.  */
}

#endif  /* _WIN32 */
