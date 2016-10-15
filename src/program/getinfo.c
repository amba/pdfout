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
#include "data.h"

static char usage[] = "PDF_FILE";
static char doc[] = "Dump the information dictionary to standard output.\n";


static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "write output to PDF_FILE.info"},
  {0}
};

static fz_context *ctx;
static char *pdf_filename;
static FILE *output;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
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
	output = open_default_write_file (ctx, pdf_filename, ".info");
      else
	output = stdout;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child children[] = {
  {&pdfout_general_argp, 0, NULL, 0},
  {0}
};

static struct argp argp = {options, parse_opt, usage, doc, children};

void
pdfout_command_getinfo (fz_context *ctx_arg, int argc, char **argv)
{
  --argc; argv = &argv[1];
  pdfout_data *hash;
  pdf_document *doc;

  ctx = ctx_arg;
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  doc = pdf_open_document (ctx, pdf_filename);

  hash = pdfout_info_dict_get (ctx, doc);

  fz_output *out = fz_new_output_with_file_ptr (ctx, output, false);
  pdfout_emitter *emitter = pdfout_emitter_json_new (ctx, out);
    
  pdfout_emitter_emit (ctx, emitter, hash);

  exit (0);
}
