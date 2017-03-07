#include "common.h"

#ifdef _WIN32 /* assume Windows NT */

/* This is a very mutch simplified/reduced version of the tmpfile windows
   implementation in GNU Gnulib, written by Ben Pfaff.  */

#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

FILE *
pdfout_tmpfile (fz_context *ctx)
{
  /* Get directory for temporary files.  */
  char tmpdir[PATH_MAX];
  if (GetTempPath (PATH_MAX, tmpdir) <= 0)
    pdfout_throw_errno (ctx, "GetTempPath failed in pdfout_tmpfile");

  /* Creat temporary filename.  */
  char filename[PATH_MAX];
  if (GetTempFileName (tmpdir, "pdfout", 0, filename) == 0)
    pdfout_throw_errno (ctx, "GetTempFileName failed in pdfout_tmpfile");

  /* Create file handle. Cannot use _O_EXCL as file is already opened by
     GetTempFileName.  */
  int fd = _open(filename, _O_CREAT | _O_TEMPORARY | _O_RDWR | _O_BINARY,
                 _S_IREAD | _S_IWRITE);
  if (fd < 0)
    pdfout_throw_errno (ctx, "_open failed in pdfout_tmpfile");

  FILE *result = _fdopen (fd, "w+b");
  if (result == NULL)
    pdfout_throw_errno (ctx, "_fdopen failed in pdfout_tmpfile");

  return result;
}



#else  /* !_WIN32 (Posix)  */

FILE *
pdfout_tmpfile (fz_context *ctx)
{
  FILE *tmp = tmpfile ();
  if (tmp == NULL)
    pdfout_throw_errno (ctx, "tmpfile failed");
  return tmp;
}

#endif


