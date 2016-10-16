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

static const char usage[] = "PDF_FILE [-o OUTPUT_FILE]";
static const char args_doc[] = "Overwrite PDF_FILE with the repaired file\n";
static struct argp_option options[] = {
  {"output", 'o', "FILE", 0,
   "write output to FILE"},
  {"check", 'c', 0, 0,
"only check PDF_FILE's integrity. If it is broken, exit with non-zero status"},
  {0}
};

static fz_context *ctx;
static char *pdf_filename;
static char *output_filename;
static bool check;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'c': check = true; break;
      
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
pdfout_command_repair (fz_context *ctx_arg, int argc, char **argv)
{
  pdf_document *doc;
  ctx = ctx_arg;
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  doc = pdf_open_document (ctx, pdf_filename);

  if (doc->repair_attempted == 0)
    {
      pdfout_msg ("file '%s' is ok", pdf_filename);
      exit (0);
    }
  
  if (check == true)
    {
      pdfout_msg ("file '%s' is broken", pdf_filename);
      exit (1);
    }
  
  if (output_filename == NULL)
    output_filename = pdf_filename;

  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  pdfout_output_to_msg (output_filename);
  exit (0);
}
