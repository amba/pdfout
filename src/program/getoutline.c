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

static char usage[] = "PDF_FILE";
static char doc[] = "Dump outline to standard output.\n";

static struct argp_option options[] = {
  /* {PDFOUT_OUTLINE_FORMAT_OPTION}, */
  {"default-filename", 'd', 0, 0,
   "write output to file PDF_FILE.outline.FORMAT"},
  {0}
};

static char *pdf_filename;
static FILE *output;
/* enum pdfout_outline_format format; */


static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
    /* case 'f': format = pdfout_outline_get_format (state, arg); break; */
    case 'd': use_default_filename = true; break;
      
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	pdf_filename = arg;
      else
	return ARGP_ERR_UNKNOWN;
      break;
      
    case ARGP_KEY_NO_ARGS:
      argp_usage (state);
      break;
      
    case ARGP_KEY_END:
      if (use_default_filename)
	output = open_default_write_file (state, pdf_filename, ".outline");
      /* 					  pdfout_outline_suffix (format)); */
      /* else */
	output = stdout;
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static const struct argp_child children[] = {
  {&pdfout_general_argp, 0, NULL, 0},
  {0}
};
static struct argp argp = {options, parse_opt, usage, doc, children};

void
pdfout_command_getoutline (int argc, char **argv)
{
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  fz_context *ctx = pdfout_new_context ();
  pdf_document *doc = pdfout_pdf_open_document (ctx, pdf_filename);

  pdfout_data *outline = pdfout_outline_get (ctx, doc);

  fz_output *out = fz_new_output_with_file_ptr (ctx, output, false);
  pdfout_emitter *emitter = pdfout_emitter_json_new (ctx, out);
  
  pdfout_emitter_emit (ctx, emitter, outline);
  
  exit (0);
}

