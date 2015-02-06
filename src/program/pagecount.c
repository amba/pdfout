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
#include "shared.h"

static const char usage[] = "PDF_FILE";
static const char args_doc[] = "Get page count\n";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, "write pagecount to file (default: stdout) FILE"},
  {0}
};

/* arguments */
static char *pdf_filename;
static char *output_filename;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o': output_filename = arg; break;

    case ARGP_KEY_ARG:
      switch (state->arg_num)
	{
	case 0: pdf_filename = arg; break;
	default: return ARGP_ERR_UNKNOWN;
	}
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage (state);

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static const struct argp_child children[] = {{&pdfout_general_argp},{0}};
static struct argp argp = {options, parse_opt, usage, args_doc, children};

void
pdfout_command_pagecount (int argc, char **argv)
{
  fz_context *ctx;
  pdf_document *doc;
  FILE *output;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  if (output_filename == NULL)
    output = stdout;
  else
    output = pdfout_xfopen (output_filename, "w");
      
  fprintf (output, "%d\n", pdf_count_pages (ctx, doc));
  
  exit (0);
}
