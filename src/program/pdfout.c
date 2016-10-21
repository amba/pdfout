#include "common.h"
#include "shared.h"

char *pdfout_program_name = NULL;

/* setup table of commands from commands.def.  */

enum command_type {
  REGULAR,
  DEBUG,
};

struct command {
  const char *name;
  enum command_type type;
  command_func_t *function;
  const char *description;
};

static const char * command_name_list[] = {
#define DEF_COMMAND(name, type, function, description) \
  name,
#include "commands.def"
  NULL
};
#undef DEF_COMMAND

static const struct command command_list[] = {
#define DEF_COMMAND(name, type, function, description)	\
  {name, type, function, description},
#include "commands.def"
  {0}
};


/* top level help and usage */

static void
print_usage ()
{
  fprintf (stderr, "Try '%s --help' for more information.\n",
	   pdfout_program_name);
}

static void
print_version ()
{
  printf ("pdfout version %s\n", PDFOUT_VERSION);
}


static struct option longopts[] = {
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'V'},
  {"usage", no_argument, NULL, 'u'},
  {"list-commands", no_argument, NULL, 'l'},
  {"describe-commands", no_argument, NULL, 'd'},
  {NULL, 0, NULL, 0},
};

static void
print_help ()
{
  printf ("Usage: %s SUBCOMMAND [PDF_FILE] ...\n\
or %s -hVld\n", pdfout_program_name, pdfout_program_name);

  puts("\n\
 Options:\n\
  -d, --describe-commands    describe all supported commands\n\
  -l, --list-commands        list all supported command names\n\
  -h, --help                 give this help list\n\
  -V, --version              print program version\
");
}

static void list_commands (void);
static void describe_commands ();

static void
parse_options (int argc, char **argv)
{
  int optc;
  while ((optc = getopt_long (argc, argv, "hVuld", longopts, NULL)) != -1)
    {
      switch (optc)
	{
	case 'h':
	  print_help();
	  break;
	case 'u':
	  print_usage();
	  break;
	case 'V':
	  print_version();
	  break;
	case 'l':
	  list_commands();
	  break;
	case 'd':
	  describe_commands();
	  break;
	default:
	  print_usage();
	  exit (1);
	}
    }
}

int
main (int argc, char **argv)
{
  if (argc == 1)
    {
      print_usage();
      exit (1);
    }
  else if (argv[1][0] == '-')
    {
      /* First argument not a command name but an option.  */
      parse_options (argc, argv);
      exit (0);
    }

  fz_context *ctx = pdfout_new_context ();

  int name_len = strlen (argv[0]) + strlen(argv[1]) + 2;
  pdfout_program_name = fz_malloc (ctx, name_len);
  pdfout_snprintf_imp (ctx, pdfout_program_name, name_len, "%s %s",
		       argv[0], argv[1]);
  
  int result = strmatch (argv[1], command_name_list);
  
  if (result < 0)
    error (1, 0, "invalid command '%s'.\n"
	   "Try '%s -l' for the list of allowed commands.", argv[1],
	   argv[0]);

  argv[1] = pdfout_program_name;
  command_list[result].function (ctx, --argc, ++argv);
  exit (0);
}

static void
list_commands (void)
{
  const struct command *command;
  
  for (command = &command_list[0]; command->name != NULL; ++command)
      if (command->type == REGULAR)
	puts (command->name);
}

static void
describe_commands ()
{
  const struct command *command;
  printf ("This build of pdfout supports the following commands:\n");

  for (command = &command_list[0]; command->name != NULL; ++command)
    if (command->type == REGULAR)
      printf ("\n%s: %s\n", command->name, command->description);
  
}


