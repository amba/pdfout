#include "shared.h"
#include "libyaml-wrappers.h"
#include "test.h"
#include <yaml.h>
#include <xalloc.h>
#include <exitfail.h>

static void
parser_initialize (yaml_parser_t *parser)
{
  if (yaml_parser_initialize (parser) == 0)
    error (exit_failure, errno, "yaml_parser_initialize");
}

/* Default style parameters. Can be changed on command line.  */
int yaml_emitter_indent = 4;
int yaml_emitter_line_width = -1;
bool yaml_emitter_escape_unicode = false;

static void
emitter_initialize (yaml_emitter_t *emitter)
{
  if (yaml_emitter_initialize (emitter) == 0)
    error (exit_failure, errno, "emitter_initialize");
  
  yaml_emitter_set_indent (emitter, yaml_emitter_indent);
  yaml_emitter_set_width (emitter, yaml_emitter_line_width);
  yaml_emitter_set_unicode (emitter, yaml_emitter_escape_unicode == false);
}

yaml_parser_t *
yaml_parser_new (FILE *input)
{
  yaml_parser_t *parser = XMALLOC (yaml_parser_t);
  parser_initialize (parser);
  if (input)
    yaml_parser_set_input_file (parser, input);
  
  return parser;
}

void yaml_parser_free (yaml_parser_t *parser)
{
  yaml_parser_delete (parser);
  free (parser);
}

yaml_emitter_t *yaml_emitter_new (FILE *output)
{
  yaml_emitter_t *emitter = XMALLOC (yaml_emitter_t);
  emitter_initialize (emitter);
  if (output)
    yaml_emitter_set_output_file (emitter, output);
  return emitter;
}

void yaml_emitter_free (yaml_emitter_t *emitter)
{
  yaml_emitter_delete (emitter);
  free (emitter);
}

/* Used in the tests. */

static int write_handler (void *data, unsigned char *buffer, size_t size)
{
  static char **string_ptr;
  static size_t len, cap;
  
  if (* (char **) data == NULL)
    {
      string_ptr = data;
      len = cap = 0;
    }
  
  if (len + size + 1 > cap) 	/* + 1 for the null-byte.  */
    {
      cap = len + size + 1;
      *string_ptr = x2nrealloc (*string_ptr, &cap, 1);
    }
  
  memcpy (*string_ptr + len, buffer, size);
  len += size;
  (*string_ptr)[len] = 0;
  
  return 1;
}

yaml_emitter_t *
test_new_emitter (char **output)
{
  yaml_emitter_t *emitter = yaml_emitter_new (NULL);
  yaml_emitter_set_output (emitter, write_handler, output);
  *output = NULL;
  return emitter;
}

void
test_reset_emitter_output (char **output)
{
  free (*output);
  *output = NULL;
}

static void
set_parser_input (yaml_parser_t *parser, const char *input)
{
  yaml_parser_set_input_string (parser, (const unsigned char *) input, strlen (input));
}

yaml_parser_t *
test_new_parser(const char *input)
{
  yaml_parser_t *parser = yaml_parser_new (NULL);
  if (input)
    set_parser_input (parser, input);
  return parser;
}

void
test_reset_parser_input (yaml_parser_t *parser, const char *input)
{
  yaml_parser_delete (parser);
  parser_initialize (parser);
  set_parser_input (parser, input);
}
    

