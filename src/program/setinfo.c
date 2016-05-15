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
#include "data.h"
#include "info-dict.h"
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
static char *pdf_output_filename;
static bool append;
static bool remove_info;
static FILE *input;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
    case 'a': append = true; break;
    case 'o': pdf_output_filename = arg; break;
    case 'r': remove_info = true; break;
    case 'd': use_default_filename = true; break;
      
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	pdf_filename = arg;
      else
	return ARGP_ERR_UNKNOWN;
      break;

    case ARGP_KEY_NO_ARGS:
      argp_usage (state);

    case ARGP_KEY_END:
      if (use_default_filename)
	input = open_default_read_file (state, pdf_filename, ".info");
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
pdfout_command_setinfo (int argc, char **argv)
{
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  fz_context *ctx = pdfout_new_context ();
  pdf_document *doc = pdfout_pdf_open_document (ctx, pdf_filename);

  pdfout_data *info = NULL;
  
  if (remove_info == false)
    {
      fz_stream *stm = fz_open_file_ptr (ctx, input);
      pdfout_parser *parser = pdfout_parser_json_new (ctx, stm);
      info = pdfout_parser_parse (ctx, parser);
    }

  pdfout_info_dict_set (ctx, doc, info, append);
  
  pdfout_write_document (ctx, doc, pdf_filename, pdf_output_filename);
  exit (0);
}
