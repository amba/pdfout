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
#include <ctype.h>
#include <progname.h>

static char usage[] = "[-f FROM_FORMAT] [-t TOO_FORMAT] [INFILE] [-o OUTFILE]";
static char doc[] = "Convert outline format\n"
  "Reads from standard input if INFILE is not given. FROM_FORMAT and\
 TOO_FORMAT may be abbreviated\n";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, "write output to FILE"},
  {"from-format", 'f', PDFOUT_OUTLINE_FORMATS, 0,
   "input format (default: YAML)", 2},
  {"to-format", 't', PDFOUT_OUTLINE_FORMATS, 0,
   "output format (default: YAML)", 2},
  {0}
};

static char *input_filename;
static char *output_filename;
enum pdfout_outline_format from, to;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 'f': from = pdfout_outline_get_format (state, arg); break;
    case 't': to = pdfout_outline_get_format (state, arg); break;
      
    case ARGP_KEY_ARG:
      if (state->arg_num == 0)
	{
	  input_filename = arg;
	  break;
	}
      /* fallthrough */
	  
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
pdfout_command_convert_outline (int argc, char **argv)
{
  yaml_document_t *doc;
  FILE *input, *output;

  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  if (input_filename)
    input = pdfout_xfopen (input_filename, "r");
  else
    input = stdin;

  if (output_filename)
    output = pdfout_xfopen (output_filename, "w");
  else
    output = stdout;
  
  if (pdfout_outline_load (&doc, input, from))
    exit (EX_DATAERR);

  if (pdfout_outline_dump (output, doc, to))
    exit (1);

  exit (0);
}
