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

static char usage[] = "[-o OUTPUT_PDF]";
static char doc[] = "Create empty PDF file\n"
  "Paper size defaults to A4\n";

static struct argp_option options[] = {
  {"output", 'o', "FILE", 0, "Write output to FILE (default: stdout)"},
  {"pages", 'p', "NUM", 0, "Create NUM pages (default: 1)"},
  {"paper-size", 's', "SIZE", 0, "use one of the following paper sizes:"},
  {"ISO 216: A0, A1, ..., A10, B0, ..., B10", 0, 0, OPTION_DOC, 0, 2},
  {"DIN extensions to ISO 216: 2A0, 4A0", 0, 0, OPTION_DOC, 0, 3},
  {"ISO 269: C0, ... C10, DL, C7/C6, C6/C5, E4" , 0, 0, OPTION_DOC, 0, 4},
  {"ANSI/ASME Y14.1: 'ANSI A' (us letter), ... , 'ANSI E'",
   0, 0, OPTION_DOC, 0, 5},
  {"landscape", 'l', 0, 0, "use landscape versions of the above formats", 6},
  {0, 0, 0, 0, "manual paper size selection: (1 pt = 1/72 in = 0.352778 mm)"},
  {"height", 'h', "FLOAT", 0, "set height to FLOAT pt"},
  {"width", 'w', "FLOAT", 0, "set width to FLOAT pt"},
  {"use either option 'paper-size' (and optionally 'landscape')"
   " or both of the 'width' and 'height' options." , 0, 0, OPTION_DOC},
  {0}
};

static char *output_filename;
static int page_count = 1;
static float height, width;

static void get_size (struct argp_state *state, char *paper_size,
		      float *width, float *height);
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static char *paper_size;
  static bool landscape;
  switch (key)
    {
    case 'o': output_filename = arg; break;
    case 's': paper_size = arg; break;
    case 'l': landscape = true; break;
      
    case 'p':
      page_count = pdfout_strtoint_null (arg);
      if (page_count < 0)
	argp_error (state, "argument of option --page has to be positiv");
      break;
      
    case 'h':
      height = pdfout_strtof (arg);
      if (height <= 0)
	argp_error (state, "height must be positive");
      break;
      
    case 'w':
      width = pdfout_strtof (arg);
      if (width <= 0)
	argp_error (state, "width must be positive");
      break;
      
    case ARGP_KEY_END:

      if (!height != !width)
	argp_error (state, "need both of the 'height' and 'width' options");

      if (paper_size && height)
	argp_error (state, "use either option 'paper-size'"
		    " or options 'height' and 'width'");

      if (height == 0 && paper_size == NULL)
	/* set default */
	paper_size = "A4";
      
      if (paper_size)
	{
	  get_size (state, paper_size, &width, &height);
	  if (landscape)
	    {
	      float tmp = width;
	      width = height;
	      height = tmp;
	    }
	}
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

static void add_pages (fz_context *ctx, pdf_document *doc, int page_count);

void
pdfout_command_create (int argc, char **argv)
{
  fz_context *ctx;
  pdf_document *doc;
  
  pdfout_argp_parse (&argp, argc, argv, 0, 0, 0);

  ctx = pdfout_new_context ();
  doc = pdf_create_document (ctx);

  add_pages (ctx, doc, page_count);

  if (output_filename == NULL)
    output_filename = "/dev/stdout";
  
  pdfout_write_document (ctx, doc, NULL, output_filename);

  exit (0);
}

static void
add_pages (fz_context *ctx, pdf_document *doc, int page_count)
{
  fz_rect rect = {0, height, width, 0};

  /* FIXME: what is the 4th arg 'res' ? */
  pdf_page *page = pdf_create_page (ctx, doc, rect, 0, 0);
  int i;

  for (i = 0; i < page_count; ++i)
    pdf_insert_page (ctx, doc, page, i);

  free (page);
}

static float mm_to_pt (float mm)
{
  float inches = mm / 25.4;
  return inches * 72;
}


static void get_iso_paper_size (int n, int width0, int height0, float *width,
				float *height)
{
  if (n % 2 == 0)
    {
      *width = mm_to_pt (floor (width0 / exp2 (n / 2)));
      *height = mm_to_pt (floor (height0 / exp2 (n / 2)));
    }
  else
    {
      *width = mm_to_pt (floor (height0 / exp2 ((n + 1) / 2)));
      *height = mm_to_pt (floor (width0 / exp2 ((n - 1) / 2)));
    }
}

static void get_ansi_paper_size (float width_in, float height_in,
				 float *width_pt, float *height_pt)
{
  *width_pt = width_in * 72;
  *height_pt = height_in * 72;
}

#define DIE(state, size) argp_error (state, "unknown paper size '%s'", size)

static void
get_size (struct argp_state *state, char *paper_size, float *width,
	  float *height)
{
  char *upcased = pdfout_upcase_ascii (paper_size);

  /* ISO 216 / 269 */

  if (strlen (upcased) <= 3
      && (upcased[0] == 'A' || upcased[0] == 'B' || upcased[0] == 'C'))
    {
      char *tailptr;
      int width0, height0;	/* size of A0/B0/C0 in mm */
      int n = pdfout_strtoint (upcased + 1, &tailptr);
      if (tailptr[0] != '\0' || n < 0 || n > 10)
	DIE (state, paper_size);
      if (upcased[0] == 'A')
	width0 = 841, height0 = 1189;
      else if (upcased[0] == 'B')
	width0 = 1000, height0 = 1414;
      else
	width0 = 917, height0 = 1297;
      
      get_iso_paper_size (n, width0, height0, width, height);
    }
  
  else if (strcmp (upcased, "2A0") == 0 || strcmp (upcased, "4A0") == 0)
    {
      int n;
      if (upcased[0] == '2')
	n = -1;
      else
	n = -2;

      get_iso_paper_size (n, 841, 1189, width, height);
    }
  
  else if (strcmp (upcased, "DL") == 0)
    get_iso_paper_size (0, 110, 220, width, height);
  
  else if (strcmp (upcased, "C7/C6") == 0)
    get_iso_paper_size (0, 81, 162, width, height);
  
  else if (strcmp (upcased, "C6/C5") == 0)
    get_iso_paper_size (0, 114, 229, width, height);

  else if (strcmp (upcased, "E4") == 0)
    get_iso_paper_size (0, 280, 400, width, height);

  /* ANSI */
  
  else if (strcmp (upcased, "ANSI A") == 0)
    get_ansi_paper_size (8.5, 11., width, height);

  else if (strcmp (upcased, "ANSI B") == 0)
    get_ansi_paper_size (11., 17., width, height);

  else if (strcmp (upcased, "ANSI C") == 0)
    get_ansi_paper_size (17., 22., width, height);

  else if (strcmp (upcased, "ANSI D") == 0)
    get_ansi_paper_size (22., 34., width, height);

  else if (strcmp (upcased, "ANSI E") == 0)
    get_ansi_paper_size (34., 44., width, height);

  else
    DIE (state, paper_size);
  
  free (upcased);
}
