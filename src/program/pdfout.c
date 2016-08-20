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

/*
   suggested reading on argp:
   Ben Asselstine: Step-by-Step into Argp
   (https://savannah.nongnu.org/projects/argpbook)

   glibc manual: Parsing Program Options with Argp
   (https://www.gnu.org/software/libc/manual/html_node/Argp.html) */

/* top level help and usage */

static struct argp_option options[] = {
  {"list-commands", 'l', 0, 0, "list all supported command names"},
  {"describe-commands", 'd', 0, 0, "describe all supported commands"},
  {"help", 'h', 0, 0, "give this help list", -1},
  {"usage", 'u', 0, OPTION_HIDDEN, "", -1},
  {"version", 'V', 0, 0, "print program version", -1},
  {0}
};

static void list_commands (void);
static void describe_commands (char *prog_name);

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  const char try_help[] = "Try '%s --help' for more information.\n";
  switch (key)
    {
    case 'l':
      list_commands ();
      exit (0);
      
    case 'd':
      describe_commands (state->name);
      exit (0);
      
    case 'h':
      fprintf (state->out_stream, "Usage: %s SUBCOMMAND [PDF_FILE] ...\n"
	       "  or: %s -dlhV\n\n", state->name, state->name);
      argp_state_help (state, state->out_stream,
		       ARGP_HELP_LONG | ARGP_HELP_EXIT_OK
		       | ARGP_HELP_DOC | ARGP_HELP_BUG_ADDR);
      exit (0);
      
    case 'V':
      fprintf (state->out_stream, "%s\n", argp_program_version);
      exit (0);
      
    case 'u':
      fprintf (state->out_stream, try_help, state->name);
      exit (0);
      
    case ARGP_KEY_END:
      fprintf (state->out_stream, try_help, state->name);
      exit (1);
      
    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

static struct argp argp = {options, parse_opt};

int
main (int argc, char **argv)
{
  /* argp_program_version = PDFOUT_VERSION; */
  /* argp_program_bug_address = PACKAGE_BUGREPORT; */

  
  if (argc < 2)
    {
      pdfout_argp_parse (&argp, argc, argv, ARGP_NO_HELP, NULL, NULL);
      exit (1);
    }
  else if (argv[1][0] == '-')
    {
      /* we provide our own --help option */
      pdfout_argp_parse (&argp, argc, argv,  ARGP_NO_HELP, NULL, NULL);
      exit (0);
    }
  
  int result = strmatch (argv[1], command_name_list);
  
  if (result < 0)
    error (1, 0, "invalid command '%s'.\n"
	   "Try '%s -l' for the list of allowed commands.", argv[1],
	   argv[0]);

  /* use the full command name in argv[1] */
  if (asprintf (&argv[1], "%s %s", argv[0],
		command_name_list[result]) == -1)
    error (1, errno, "asprintf");

  /* finally call the command... */
  fz_context *ctx = pdfout_new_context ();
  command_list[result].function (ctx, argc - 1, &argv[1]);
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
describe_commands (char *prog_name)
{
  const struct command *command;
  printf ("This build of pdfout supports the following commands:\n");
  printf ("(Try '%s COMMAND --help' to learn more about COMMAND.)\n",
	  prog_name);

  for (command = &command_list[0]; command->name != NULL; ++command)
    if (command->type == REGULAR)
      printf ("\n%s: %s\n", command->name, command->description);
  
}


