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
static char doc[] = "Read the new outline from standard input.\n";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, PDFOUT_NO_INCREMENTAL},
  /* {PDFOUT_OUTLINE_FORMAT_OPTION}, */
  {"default-filename", 'd', 0, 0, "read input from PDF_FILE.outline.FORMAT"},
  {"remove", 'r', 0, 0, "remove outline"},
  {0}
};

static char *pdf_filename;
static char *output_filename;
static FILE *input;
static bool remove_outline;
/* static enum pdfout_outline_format format; */

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'd': use_default_filename = true; break;
    case 'r': remove_outline = true; break;
    /* case 'f': format = pdfout_outline_get_format (state, arg); break; */
      
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
	input = open_default_read_file (state, pdf_filename, ".outline");
					/* pdfout_outline_suffix (format)); */
      else
	input = stdin;
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
pdfout_command_setoutline (int argc, char **argv)
{
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  fz_context *ctx = pdfout_new_context ();
  pdf_document *doc = pdfout_pdf_open_document (ctx, pdf_filename);

  pdfout_data *outline;
  if (remove_outline == false)
    {
      fz_stream *stm = fz_open_file_ptr (ctx, input);
      pdfout_parser *parser = pdfout_parser_json_new (ctx, stm);
      outline = pdfout_parser_parse (ctx, parser);
    }
  else
    outline = NULL;
  

  pdfout_outline_set (ctx, doc, outline);
  
  pdfout_write_document (ctx, doc, pdf_filename, output_filename);

  exit (0);
}
