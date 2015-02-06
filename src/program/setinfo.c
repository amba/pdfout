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

static char usage[] = "PDF-FILE [INFO-FILE]";
static char doc[] = "Modify the information dictionary\n";

static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "read input from PDF_FILE.info"},
  {"output", 'o', "FILE", 0, PDFOUT_NO_INCREMENTAL},
  {"remove", 'r', 0, 0, "remove all entries"},
  {"append", 'a', 0, 0, "do not remove existing keys"},
  {0}
};

static char *pdf_filename;
static char *info_filename;
static char *output_filename;
static bool append;
static bool remove_info;
static bool use_default_filename;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  
  switch (key)
    {
    case 'a': append = true; break;
    case 'o': output_filename = arg; break;
    case 'r': remove_info = true; break;
    case 'd': use_default_filename = true; break;
      
    case ARGP_KEY_ARG:
      switch (state->arg_num)
	{
	case 0: pdf_filename = arg; break;
	case 1: info_filename = arg; break;
	default: return ARGP_ERR_UNKNOWN;
	}
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage (state);

    case ARGP_KEY_END:
      if (info_filename && use_default_filename)
	argp_error (state, "use either option '-d' or second argument");
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child children[] = {
      {&pdfout_pdf_output_argp, 0, "", -2},
      {&pdfout_general_argp, 0, NULL, 0},
      {0}
};

static struct argp argp = {options, parse_opt, usage, doc, children};

void
pdfout_command_setinfo (int argc, char **argv)
{
  yaml_document_t *yaml_doc = NULL;
  fz_context *ctx;
  pdf_document *doc;
  FILE *input;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  if (remove_info == false)
    {
      input = pdfout_get_stream (&info_filename, 'r', pdf_filename,
				 use_default_filename, ".info");
  
      if (pdfout_load_yaml (&yaml_doc, input))
	exit (1);
    }
  
  if (pdfout_update_info_dict (ctx, doc, yaml_doc, append))
    exit (EX_DATAERR);

  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  exit (0);
}
