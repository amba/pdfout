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

#include "shared.h"
#include "../page-labels.h"
static char usage[] = "PDF_FILE";
static char doc[] = "Read page labels from standard input.\n";

static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0, "read input from PDF_FILE.pagelabels"},
  {"output", 'o', "FILE", 0, PDFOUT_NO_INCREMENTAL},
  {"remove", 'r', 0, 0, "remove page labels"},
  {0}
};

static char *pdf_filename;
static char *output_filename;
static FILE *input;
static bool remove_page_labels;

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'r': remove_page_labels = true; break;
    case 'd': use_default_filename = true; break;

      break;
      
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
	input = open_default_read_file (state, pdf_filename, ".pagelabels");
      else
	input = stdin;
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
  pdf_document *doc;
  yaml_parser_t *parser;
  pdfout_page_labels_t *labels;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  if (remove_page_labels == false)
    {
      parser = yaml_parser_new (input);
      if (pdfout_page_labels_from_yaml (&labels, parser))
	exit (EX_DATAERR);
    }
  else
    labels = NULL;
  
  if (pdfout_page_labels_set (ctx, doc, labels))
    exit (EX_DATAERR);
  
  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  exit (0);
}
