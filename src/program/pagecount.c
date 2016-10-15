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
static const char args_doc[] = "Write pagecount to standard output.\n";

static struct argp_option options[] = {
  {0}
};

/* arguments */
static fz_context *ctx;
static char *pdf_filename;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	pdf_filename = arg;
      else
	return ARGP_ERR_UNKNOWN;
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
pdfout_command_pagecount (fz_context *ctx_arg, int argc, char **argv)
{
  --argc; argv = &argv[1];
  pdf_document *doc;
  ctx = ctx_arg;
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdf_open_document (ctx, pdf_filename);

  
  printf ("%d\n", pdf_count_pages (ctx, doc));
  
  exit (0);
}
