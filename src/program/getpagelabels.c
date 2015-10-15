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
#include "../page-labels.h"

static char usage[] = "PDF_FILE";
static char doc[] = "Dump page labels to standard output.\n"

  "\vReturn values:\n\
0: PDF_FILE has valid page labels.\n\
1: PDF_FILE has no page labels. No output is produced.\n\
2: PDF_FILE's page labels are damaged, but part of them could be extracted.\n\
3: PDF_FILE's page labels are damaged. No output is produced.";


static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "write output to PDF_FILE.pagelabels"},
  {0}
};

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
	output = open_default_write_file (state, pdf_filename, ".pagelabels");
      else
	output = stdout;
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
pdfout_command_getpagelabels (int argc, char **argv)
{
  fz_context *ctx;
  pdf_document *doc;
  pdfout_page_labels_t *labels;
  yaml_emitter_t *emitter;
  int rv;

  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  rv = pdfout_page_labels_get (&labels, ctx, doc);
  
  if (labels == NULL)
    {
      pdfout_no_output_msg ();
      exit (2 * rv + 1);
    }

  emitter = yaml_emitter_new (output);
  
  if (pdfout_page_labels_to_yaml (emitter, labels))
    exit (EX_IOERR);
  
  exit (2 * rv);
}
