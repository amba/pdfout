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
static char doc[] = "Extract text and write it to standard output.\n";



static struct argp_option options[] = {
  {"default-filename", 'd', 0, 0,
   "write output to PDF_FILE.txt"},
  {"page-range", 'p', "PAGE1[-PAGE2][,PAGE3[-PAGE4]...]", 0,
   "only print text for the specified page ranges."},
  {0}
};

static char *pdf_filename;
static FILE *output;
static char *page_range;


static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static bool use_default_filename;
  switch (key)
    {
    case 'd': use_default_filename = true; break;
    case 'p': page_range = arg; break;
      
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
      if (use_default_filename)
	output = open_default_write_file (state, pdf_filename, ".txt");
      else
	output = stdout;
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp_child children[] = {
   {&pdfout_general_argp, 0, NULL, 0},
   {0}
};

static struct argp argp = {options, parse_opt, usage, doc, children};

void
pdfout_command_gettxt (int argc, char **argv)
{
  fz_context *ctx;
  pdf_document *doc;
  int  i, page_count;
  int *pages;

  /* only move a copy of pages to soothe Memcheck */
  int *pages_ptr;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);
  
  ctx = pdfout_new_context ();
  doc = pdf_open_document (ctx, pdf_filename);
  page_count = pdf_count_pages (ctx, doc);

  if (page_range == NULL) {
    pages = XNMALLOC (3, int);
    pages[0] = 1; pages[1] = page_count; pages[2] = 0;
  }
  else
    pages = pdfout_parse_page_range (page_range, page_count);
  

  for (pages_ptr = pages; pages_ptr[0]; pages_ptr += 2)
    for (i = pages_ptr[0]; i <= pages_ptr[1]; ++i)
      pdfout_text_get_page (output, ctx, doc, i - 1);
    
  exit (0);
}
