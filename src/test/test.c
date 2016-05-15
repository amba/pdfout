/* The pdfout document modification and analysis tool.
   Copyright (C) 2015 AUTHORS (see AUTHORS file)
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include "test.h" /* specifications */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <errno.h>
#include <error.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <xvasprintf.h>
#include <clean-temp.h>
#include <tempname.h>
#include <progname.h>
#include <copy-file.h>
#include <full-read.h>
#include <xalloc.h>
#ifndef GNULIB_DIRNAME
 #define GNULIB_DIRNAME 1
#endif
#include <dirname.h>


static struct temp_dir *temp_dir = NULL;
static const char *srcdir;

/* register PATH in the temporary directory */
static char *register_temp (char *path);

static void
cleanup (void)
{
  if (temp_dir && cleanup_temp_dir (temp_dir))
    abort ();
}


void
_test_init (const char *file_name)
{
  char *pdfout_valgrind_env;
  char *program_name = base_name (file_name);

  set_program_name (program_name); /* Never free program_name!  */
  srcdir = TEST_STRINGIFY (TEST_DATA_DIR);
  error (0, 0, "srcdir: %s", srcdir);

  pdfout_valgrind_env = getenv ("pdfout_valgrind");
  if (pdfout_valgrind_env && strcmp (pdfout_valgrind_env, "yes") == 0)
    use_valgrind = true;
}

static void
init_tempdir (void)
{
  if (temp_dir == NULL)
    {
      temp_dir = create_temp_dir ("pdf", NULL, 1);
      if (temp_dir == NULL)
	error (99, errno, "create_temp_dir failed");

      if (atexit (&cleanup))
	error (99, 0, "atexit");
    }
}

char *
test_tempname (void)
{
  char *tmpl;

  init_tempdir ();
  
  tmpl = xasprintf ("%s/%s", temp_dir->dir_name, "XXXXXX");
    
  if (gen_tempname (tmpl, 0, 0, GT_NOCREATE))
    error (99, errno, "tempname: gen_tempname");

  register_temp_file (temp_dir, tmpl);
  
  return tmpl;
}

static char *
register_temp (char *path)
{
  char *dirname;
  init_tempdir ();

  dirname = dir_name (path);

  if (strcmp (dirname, temp_dir->dir_name))
    error (99, 0, "filename '%s' is not in the tests temporary directory"
	   " '%s'", path, temp_dir->dir_name);
  
  register_temp_file (temp_dir, path);
  
  free (dirname);
  return path;
}



char *
test_new_pdf (void)
{
  char *pdf = test_tempname ();
  test_cp ("empty10.pdf", pdf);

  return pdf;
}

char *
test_cp (const char *file, char *tmp_file)
{
  char *path;
  
  if (tmp_file == NULL)
    tmp_file = test_tempname ();
  else
    register_temp (tmp_file);

  path = test_data (file);

  copy_file_preserving (path, tmp_file);
    
  /* during 'make distcheck' the source tree is readonly.
     So we have to make the file writable */
  if (chmod (tmp_file, S_IWUSR | S_IRUSR))
    error (1, 0, "chmod failed");

  free (path);
    
  return tmp_file;
}

char *
test_data (const char *file)
{
  if (srcdir == NULL)
    error (1, 0, "test_data: dir is NULL");
  if (file == NULL)
    error (1, 0, "test_data: file is NULL");
  
  return xasprintf ("%s/data/%s", srcdir, file);
}

#define safe_snprintf(buf, fmt, args ...)		\
  safe_snprintf_imp (buf, sizeof buf, fmt, ## args)

static void PDFOUT_PRINTFLIKE (3)
safe_snprintf_imp (char *buf, int size, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);
  int ret = vsnprintf (buf, size, fmt, ap);
  if (ret < 0 || ret >= size)
    abort ();
}

void
test_set_get (const char *command, const char *input, const char *expected,
	      const char *empty, const char **broken_inputs)
{
  char setcommand[100];
  char getcommand[100];
  
  safe_snprintf (setcommand, "%s%s", "set", command);
  safe_snprintf (getcommand, "%s%s", "get", command);

  {
    /* compare after get and set */
    char *pdf = test_new_pdf ();
    test_pdfout (input, 0, setcommand, pdf);
    test_pdfout (0, expected, getcommand, pdf);
  }

  {
    /* remove */
    char *pdf = test_new_pdf ();
    test_pdfout (input, 0, setcommand, pdf);
    test_pdfout (0, 0, setcommand, pdf, "--remove");
    test_pdfout (0, empty, getcommand, pdf);
  }

  {
    char *pdf = test_new_pdf ();
    /* Check exit status for all broken files.  */
    for (int i = 0; broken_inputs[i]; ++i)
      {
	test_pdfout_status (1, broken_inputs[i], 0, setcommand, pdf);

	/* pdf is untouched.  */
	test_files_equal (pdf, test_data ("empty10.pdf"));
      }
  }
}
