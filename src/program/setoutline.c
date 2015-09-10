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

static char usage[] = "PDF_FILE [OUTLINE]";
static char doc[] = "Modify the outline\n";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, PDFOUT_NO_INCREMENTAL},
  {PDFOUT_OUTLINE_FORMAT_OPTION},
  {"default-filename", 'd', 0, 0, "read input from PDF_FILE.outline.FORMAT"},
  {"default-view", 'v', "DEST", 0, "set default outline destination"},
  {"remove", 'r', 0, 0, "remove outline"},
  {0}
};

static char *pdf_filename;
static char *output_filename;
static char *outline_filename;
static char *default_view;
static bool use_default_filename;
static bool remove_outline;
static enum pdfout_outline_format format;

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'v': default_view = arg; break;
    case 'o': output_filename = arg; break;
    case 'd': use_default_filename = true; break;
    case 'r': remove_outline = true; break;
    case 'f': format = pdfout_outline_get_format (state, arg); break;
      
    case ARGP_KEY_ARG:
      switch (state->arg_num)
	{
	case 0: pdf_filename = arg; break;
	case 1: outline_filename = arg; break;
	default: return ARGP_ERR_UNKNOWN;
	}
      break;
      
    case ARGP_KEY_NO_ARGS:
      argp_usage (state);

    case ARGP_KEY_END:
      if (outline_filename && use_default_filename)
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

static void parse_default_view (char *default_view);

void
pdfout_command_setoutline (int argc, char **argv)
{
  pdf_document *doc;
  fz_context *ctx;
  yaml_document_t *yaml_doc = NULL;
  FILE *input;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdfout_pdf_open_document (ctx, pdf_filename);

  if (remove_outline == false)
    {
      input = pdfout_get_stream (&outline_filename, 'r', pdf_filename,
				 use_default_filename,
				 pdfout_outline_suffix (format));
  
      if (pdfout_outline_load (&yaml_doc, input, format))
	exit (EX_DATAERR);
  
      if (default_view)
	parse_default_view (default_view);
    }
  
  if (pdfout_outline_set (ctx, doc, yaml_doc))
    exit (EX_DATAERR);
      
  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  exit (0);
}

static void
parse_default_view (char *default_view)
{
  yaml_parser_t *parser = XZALLOC (yaml_parser_t);
  
  pdfout_yaml_parser_initialize (parser);
  yaml_parser_set_input_string (parser, (yaml_char_t *) default_view,
				strlen (default_view));
  pdfout_default_dest_array = XZALLOC (yaml_document_t);
  pdfout_yaml_parser_load (parser, pdfout_default_dest_array);
      
  /* cleanup  */
  yaml_parser_delete (parser);
  free (parser);
}
