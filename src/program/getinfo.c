#include "common.h"
#include "shared.h"
#include "data.h"

static fz_context *ctx;
static char *pdf_filename;
static FILE *output;

static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"usage", no_argument, NULL, 'u'},
  {"default-filename", no_argument, NULL, 'd'},
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
Dump info dict as JSON to standard output.\n\
\n\
 Options:\n\
  -d, --default-filename     Write output to PDF_FILE.info\n\
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
  while ((optc = getopt_long (argc, argv, "hud", longopts, NULL)) != -1)
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
    output = open_default_write_file (ctx, pdf_filename, ".info");
  else
    output = stdout;
}

void
pdfout_command_getinfo (fz_context *ctx_arg, int argc, char **argv)
{
  pdfout_data *hash;
  pdf_document *doc;

  ctx = ctx_arg;

  parse_options (argc, argv);
  
  doc = pdf_open_document (ctx, pdf_filename);

  hash = pdfout_info_dict_get (ctx, doc);

  fz_output *out = fz_new_output_with_file_ptr (ctx, output, false);
  pdfout_emitter *emitter = pdfout_emitter_json_new (ctx, out);
    
  pdfout_emitter_emit (ctx, emitter, hash);

  exit (0);
}
