#include "common.h"
#include "shared.h"

static char *pdf_filename;
static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
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
Print page count of PDF_FILE to standard output.\n\
\n\
 Options:\n\
 general options:\n\
  -h, --help                 Give this help list\n\
  -u, --usage                Give a short usage message\n\
");
}	

static void
parse_options (int argc, char **argv)
{
  int optc;
  while ((optc = getopt_long (argc, argv, "hu", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'h':
	  print_help ();
	  exit (0);
	case 'u':
	  print_usage ();
	  exit (0);
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
}


void
pdfout_command_pagecount (fz_context *ctx, int argc, char **argv)
{
  pdf_document *doc;
  parse_options (argc, argv);
  ctx = pdfout_new_context ();
  doc = pdf_open_document (ctx, pdf_filename);

  
  printf ("%d\n", pdf_count_pages (ctx, doc));

  pdf_drop_document (ctx, doc);
}
