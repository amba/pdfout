#include "common.h"
#include "shared.h"

static fz_context *ctx;
static char *pdf_filename;
static FILE *output;
static char *page_range;

static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
  {"default-filename", no_argument, NULL, 'd'},
  {"page-range", required_argument, NULL, 'p'},
  {NULL, 0, NULL, 0}
};

static void
print_usage ()
{
  printf ("Usage: %s [OPTIONS] PDF_FILE\n", pdfout_program_name);
}

static void
print_help ()
{
  print_usage ();
  puts ("\
Extract text and print it to stdout.\n\
\n\
 Options:\n\
  -d, --default-filename     Write output to PDF_FILE.txt\n\
  -p, --page-range=PAGE1[-PAGE2][,PAGE3[-PAGE4]...]\n\
                             Only print text for the specified page ranges\n\
\n\
 general options:\n\
  -h, --help                 Give this help list\n\
  -u, --usage                Give a short usage message\n\
");
}

static void
parse_options (int argc, char **argv)
{
  int optc;
  bool use_default_filename = false;
  while ((optc = getopt_long (argc, argv, "hudp:", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'h':
	  print_help ();
	  exit (0);
	case 'u':
	  print_usage ();
	  exit (0);
	case 'd':
	  use_default_filename = true;
	  break;
	case 'p':
	  page_range = optarg;
	  break;
	default:
	  print_usage ();
	  exit (1);
	}
    }

  if (argc - 1 < optind)
    {
      print_usage ();
      exit (1);
    }
  pdf_filename = argv[optind];

  if (use_default_filename)
    output = open_default_write_file (ctx, pdf_filename, ".txt");
  else
    output = stdout;
}

void
pdfout_command_gettxt (fz_context *ctx_arg, int argc, char **argv)
{
  pdf_document *doc;
  int  i, page_count;
  int *pages;

  /* only move a copy of pages to soothe Memcheck */
  int *pages_ptr;

  ctx = ctx_arg;
  
  parse_options(argc, argv);
  
  doc = pdf_open_document (ctx, pdf_filename);
  page_count = pdf_count_pages (ctx, doc);

  if (page_range == NULL) {
    pages = fz_malloc (ctx, 3 * sizeof (int));
    pages[0] = 1; pages[1] = page_count; pages[2] = 0;
  }
  else
    pages = pdfout_parse_page_range (ctx, page_range, page_count);
  

  for (pages_ptr = pages; pages_ptr[0]; pages_ptr += 2)
    for (i = pages_ptr[0]; i <= pages_ptr[1]; ++i)
      pdfout_text_get_page (output, ctx, doc, i - 1);
    
  exit (0);
}
