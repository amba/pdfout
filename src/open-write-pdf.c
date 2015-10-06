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


#include "common.h"
#include <tempname.h>
#include <unistd.h>
#include <copy-file.h>

pdf_document *
pdfout_pdf_open_document (fz_context *ctx, const char *file)
{
  pdf_document *rv;

  fz_try (ctx)
  {
    rv = pdf_open_document (ctx, file);
  }
  fz_catch (ctx)
  {
    pdfout_errno_msg (errno, "caught error: %s", fz_caught_message (ctx));
    exit (EX_USAGE);
  }
  
  return rv;
}

void
pdfout_write_document (fz_context *ctx, pdf_document *doc,
		       const char *pdf_filename, const char *output_filename)
{
  fz_write_options opts = { 0 };
  /* Use volatile, because of the setjmp in fz_try.  */
  char *volatile write_filename; 
  volatile bool use_tmp = false;
  
  if (output_filename == NULL)
    {
      if (doc->repair_attempted)
	{
	  pdfout_msg ("won't do incremental update for broken file");
	  exit (EX_IOERR);
	}
      opts.do_incremental = 1;
      write_filename = (char *) pdf_filename;
    }
  else
    {
      if (pdf_filename && strcmp (pdf_filename, output_filename) == 0)
	{
	  use_tmp = true;
	  write_filename = xasprintf ("%sXXXXXX", pdf_filename);
	  if (gen_tempname (write_filename, 0, 0, GT_NOCREATE))
	    error (1, errno, "gen_tempname");
	  pdfout_msg ("write_filename: '%s'", write_filename);
	  
	}
      else
	write_filename = (char *) output_filename;
    }

  /* pdf_write_filename can throw exceptions, so wrap around it */
  fz_try (ctx)
  {
    pdf_write_document (ctx, doc, write_filename, &opts);
  }
  
  fz_catch (ctx)
  {
    pdfout_errno_msg (errno, "caught error: %s", fz_caught_message (ctx));
    exit (1);
  }

  if (use_tmp)
    {
      copy_file_preserving (write_filename, output_filename);

      if (unlink (write_filename))
	{
	  pdfout_errno_msg (errno, "unlink");
	  exit (1);
	}
      
      free (write_filename);
    }
}
