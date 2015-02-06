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
static char doc[] = "Dump the information dictionary\n";

static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "write output to PDF_FILE.info"},
  {"output", 'o', "YAML", 0, "write output to file YAML (default: stdout)."},
  {0}
};

static char *pdf_filename;
static char *output_filename;
static bool use_default_filename;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'd': use_default_filename = true; break;

    case ARGP_KEY_ARG:
      switch (state->arg_num)
	{
	case 0: pdf_filename = arg; break;
	default: return ARGP_ERR_UNKNOWN;
	}
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage (state);
      
    case ARGP_KEY_END:
      if (output_filename && use_default_filename)
	argp_error (state, "options -o and -d are mutually exclusive");
      break;
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child children[] = {
  {&pdfout_yaml_emitter_argp, 0, "", -2},
  {&pdfout_general_argp, 0, NULL, 0},
  {0}
};

static struct argp argp = {options, parse_opt, usage, doc, children};

void
pdfout_command_getinfo (int argc, char **argv)
{
  yaml_document_t *yaml_doc;
  fz_context *ctx;
  pdf_document *doc;
  FILE *output;
    
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);
  
  if (pdfout_get_info_dict (&yaml_doc, ctx, doc))
    {
      pdfout_no_output_msg ();
      exit (1);
    }

  output = pdfout_get_stream (&output_filename, 'w', pdf_filename,
			      use_default_filename, ".info");
  
  pdfout_dump_yaml (output, yaml_doc);

  pdfout_output_to_msg (output_filename);
  exit (0);
}
