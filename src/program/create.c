
#include "common.h"
#include "shared.h"


static fz_context *ctx;
static int page_count = 1;
static float height, width;
static const char *output_filename = "/dev/stdout"; /* FIXME: non portable */

enum {
  HEIGHT_OPTION = 127,
};

static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
  {"output", required_argument, NULL, 'o'},
  {"pages", required_argument, NULL, 'p'},
  {"paper-size", required_argument, NULL, 's'},
  {"landscape", no_argument, NULL, 'l'},
  {"height", required_argument, NULL, HEIGHT_OPTION},
  {"width", required_argument, NULL, 'w'},
  {NULL, 0, NULL, 0}
};

static void
print_usage ()
{
  printf ("Usage: %s [OPTIONS] > OUTPUT.pdf\n", pdfout_program_name);
}

static void
print_help ()
{
  print_usage ();
  puts ("\
Create empty PDF file.\n\
Redirect to a plain file to get a valid PDF.\n\
Paper size defaults to A4\n\
\n\
 Options:\n\
  -o, --output=FILE          Write PDF to FILE\n\
  -p, --pages=NUM            Create NUM pages (default: 1)\n\
  -s, --paper-size=SIZE      use one of the following paper sizes:\n\
  ISO 216: A0, A1, ..., A10, B0, ..., B10\n\
  DIN extensions to ISO 216: 2A0, 4A0\n\
  ISO 269: C0, ... C10, DL, C7/C6, C6/C5, E4\n\
  ANSI/ASME Y14.1: 'ANSI A' (us letter), ... , 'ANSI E'\n\
  -l, --landscape            use landscape versions of the above formats\n\
\n\
 manual paper size selection: (1 pt = 1/72 in = 0.352778 mm)\n\
      --height=FLOAT         set height to FLOAT pt\n\
  -w, --width=FLOAT          set width to FLOAT pt\n\
  use either option 'paper-size' (and optionally 'landscape') or both of the\n\
  'width' and 'height' options.\n\
\n\
 general options:\n\
  -h, --help                 Give this help list\n\
  -u, --usage                Give a short usage message\n\
");
}

static void get_size (const char *paper_size, float *width, float *height);

static void
parse_options (int argc, char **argv)
{
  const char *paper_size = NULL;
  bool landscape = false;
  
  int optc;
  while ((optc = getopt_long (argc, argv, "huo:p:s:lh:w:", longopts, NULL))
	 != -1)
    {
      switch (optc)
	{
	case 'h':
	  print_help ();
	  exit (0);
	case 'u':
	  print_usage ();
	  exit (0);
	case 'o':
	  output_filename = optarg;
	  break;
	case 'p':
	  page_count = pdfout_strtoint_null (ctx, optarg);
	  break;
	case 's':
	  paper_size = optarg;
	  break;
	case 'l':
	  landscape = true;
	  break;
	case HEIGHT_OPTION:
	  height = pdfout_strtof (ctx, optarg);
	  break;
	case 'w':
	  width = pdfout_strtof (ctx, optarg);
	  break;
	  
	default:
	  print_usage ();
	  exit (1);
	}
    }

  if (page_count < 0)
    pdfout_throw (ctx, "argument of option --pages has to be positiv");

  if (height < 0 || width < 0)
    pdfout_throw (ctx, "height and width must be positive");

  if (!height != !width)
    pdfout_throw (ctx, "need both --height and --width options");

  if (paper_size && height)
    pdfout_throw (ctx, "use either option --paper-size "
	      "or options --height and --width");

  if (height == 0 && paper_size == NULL)
    /* Set to default.  */
    paper_size = "A4";

  if (paper_size)
    {
      get_size (paper_size, &width, &height);
      if (landscape)
	{
	  float tmp = width;
	  width = height;
	  height = tmp;
	}
    }
}

void
pdfout_command_create (fz_context *ctx_arg, int argc, char **argv)
{
  ctx = ctx_arg;
  parse_options (argc, argv);
  
  fz_rect rect = {0, 0, width, height};
  pdf_document *doc = pdfout_create_blank_pdf (ctx, page_count, &rect);
  
  pdfout_write_document (ctx, doc, NULL, output_filename);
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

static void
get_size (const char *paper_size, float *width, float *height)
{
  char *size_copy = fz_strdup (ctx, paper_size);

  /* ISO 216 / 269 */

  if (strlen (size_copy) <= 3
      && (size_copy[0] == 'A' || size_copy[0] == 'B' || size_copy[0] == 'C'))
    {
      char *tailptr;
      int width0, height0;	/* size of A0/B0/C0 in mm */
      int n = pdfout_strtoint (ctx, size_copy + 1, &tailptr);
      if (tailptr[0] != '\0' || n < 0 || n > 10)
	pdfout_throw (ctx, "unknown paper size '%s'", paper_size);
      if (size_copy[0] == 'A')
	width0 = 841, height0 = 1189;
      else if (size_copy[0] == 'B')
	width0 = 1000, height0 = 1414;
      else
	width0 = 917, height0 = 1297;
      
      get_iso_paper_size (n, width0, height0, width, height);
    }
  
  else if (strcmp (size_copy, "2A0") == 0 || strcmp (size_copy, "4A0") == 0)
    {
      int n;
      if (size_copy[0] == '2')
	n = -1;
      else
	n = -2;

      get_iso_paper_size (n, 841, 1189, width, height);
    }
  
  else if (strcmp (size_copy, "DL") == 0)
    get_iso_paper_size (0, 110, 220, width, height);
  
  else if (strcmp (size_copy, "C7/C6") == 0)
    get_iso_paper_size (0, 81, 162, width, height);
  
  else if (strcmp (size_copy, "C6/C5") == 0)
    get_iso_paper_size (0, 114, 229, width, height);

  else if (strcmp (size_copy, "E4") == 0)
    get_iso_paper_size (0, 280, 400, width, height);

  /* ANSI */
  
  else if (strcmp (size_copy, "ANSI A") == 0)
    get_ansi_paper_size (8.5, 11., width, height);

  else if (strcmp (size_copy, "ANSI B") == 0)
    get_ansi_paper_size (11., 17., width, height);

  else if (strcmp (size_copy, "ANSI C") == 0)
    get_ansi_paper_size (17., 22., width, height);

  else if (strcmp (size_copy, "ANSI D") == 0)
    get_ansi_paper_size (22., 34., width, height);

  else if (strcmp (size_copy, "ANSI E") == 0)
    get_ansi_paper_size (34., 44., width, height);

  else
    pdfout_throw (ctx, "unknown paper size '%s'", paper_size);
  
  free (size_copy);
}
