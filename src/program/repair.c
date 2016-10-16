#include "common.h"
#include "shared.h"

static fz_context *ctx;
static char *pdf_filename;
static char *output_filename;
static bool check;

static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
  {"output", required_argument, NULL, 'o'},
  {"check", no_argument, NULL, 'c'},
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
Overwrite PDF_FILE with the repaired file.\n\
\n\
 options:\n\
  -c, --check                only check PDF_FILE's integrity.\n\
                             Exit with non-zero status, if broken\n\
  -o, --output=FILE          write output to FILE\n\
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
  while ((optc = getopt_long (argc, argv, "huo:c", longopts, NULL)) != -1)
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
	case 'c':
	  check = true;
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
}

void
pdfout_command_repair (fz_context *ctx_arg, int argc, char **argv)
{
  pdf_document *doc;
  ctx = ctx_arg;

  parse_options (argc, argv);
  doc = pdf_open_document (ctx, pdf_filename);

  if (doc->repair_attempted == 0)
    {
      pdfout_msg ("file '%s' is ok", pdf_filename);
      exit (0);
    }
  
  if (check == true)
    {
      pdfout_msg ("file '%s' is broken", pdf_filename);
      exit (1);
    }
  
  if (output_filename == NULL)
    output_filename = pdf_filename;

  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
  pdfout_output_to_msg (output_filename);
  exit (0);
}
