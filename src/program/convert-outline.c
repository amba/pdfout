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

static char usage[] = "[-f FROM_FORMAT] [-t TOO_FORMAT]";
static char doc[] = "Convert outline format\n"
  "Reads outline in FROM_FORMAT from standard input and writes outline in \
TOO_FORMAT to standard output.\n";

static struct argp_option options[] = {
  {"from-format", 'f', PDFOUT_OUTLINE_FORMATS, 0,
   "input format (default: YAML)", 2},
  {"to-format", 't', PDFOUT_OUTLINE_FORMATS, 0,
   "output format (default: YAML)", 2},
  {0}
};

enum pdfout_outline_format from, to;

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f': from = pdfout_outline_get_format (state, arg); break;
    case 't': to = pdfout_outline_get_format (state, arg); break;
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

  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  if (pdfout_outline_load (&doc, stdin, from))
    exit (EX_DATAERR);

  if (pdfout_outline_dump (stdout, doc, to))
    exit (1);

  exit (0);
}
