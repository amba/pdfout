#include "common.h"
#include "shared.h"

static fz_context *ctx;
static char *pdf_filename;
static char *output_filename;
static FILE *input;
static bool remove_page_labels;

static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
  {"default-filename", no_argument, NULL, 'd'},
  {"output", required_argument, NULL, 'o'},
  {"remove", no_argument, NULL, 'r'},
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
Modify page labels. Reads JSON from stdin\n\
\n\
 Options:\n\
  -d, --default-filename     Write output to PDF_FILE.pagelabels\n\
  -o, --output=FILE          Write modified document to FILE\n\
  -r, --remove               Remove page labels\n\
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
  while ((optc = getopt_long (argc, argv, "hudo:r", longopts, NULL)) != -1)
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
	case 'o':
	  output_filename = optarg;
	  break;
	case 'r':
	  remove_page_labels = true;
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
    input = open_default_read_file (ctx, pdf_filename, ".pagelabels");
  else
    input = stdin;
}

void
pdfout_command_setpagelabels (fz_context *ctx_arg, int argc, char **argv)
{
  ctx = ctx_arg;

  parse_options (argc, argv);
  
  pdf_document *doc = pdf_open_document (ctx, pdf_filename);

  pdfout_data *labels;
  if (remove_page_labels == false)
    {
      fz_stream *stm = fz_open_file_ptr (ctx, input);
      pdfout_parser *parser = pdfout_parser_json_new (ctx, stm);
      labels = pdfout_parser_parse (ctx, parser);
      fz_drop_stream (ctx, stm);
    }
  else
    labels = NULL;
  

  pdfout_page_labels_set (ctx, doc, labels);
  pdfout_data_drop (ctx, labels);
  
  pdfout_write_document (ctx, doc, pdf_filename, output_filename);
}
