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

static char usage[] = "PDF_FILE [YAML]";
static char doc[] = "Modify page labels\n";

static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "read input from PDF_FILE.pagelabels"},
  {"output", 'o', "FILE", 0, PDFOUT_NO_INCREMENTAL},
  {"remove", 'r', 0, 0, "remove page labels"},
  {0}
};

static char *pdf_filename;
static char *output_filename;
static char *page_labels_filename;
static bool use_default_filename;
static bool remove_page_labels;

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'd': use_default_filename = true; break;
    case 'r': remove_page_labels = true; break;
      
    case ARGP_KEY_ARG:
      switch (state->arg_num)
	{
	case 0: pdf_filename = arg; break;
	case 1: page_labels_filename = arg; break;
	default: return ARGP_ERR_UNKNOWN;
	}
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage (state);

    case ARGP_KEY_END:
      if (page_labels_filename && use_default_filename)
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
pdfout_command_setpagelabels (int argc, char **argv)
{
  fz_context *ctx;
  yaml_document_t *yaml_doc = NULL;
  pdf_document *doc;
  FILE *input;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  if (remove_page_labels == false)
    {
      input = pdfout_get_stream (&page_labels_filename, 'r', pdf_filename,
				 use_default_filename, ".pagelabels");
      
      if (pdfout_load_yaml (&yaml_doc, input))
	exit (1);
    }

  if (pdfout_update_page_labels (ctx, doc, yaml_doc))
    exit (EX_DATAERR);
      
  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  exit (0);
}
