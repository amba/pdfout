#include "common.h"

typedef struct {
  pdfout_emitter super;
  fz_output *out;
  bool finished;
  int indent_level;
} emitter;

static void
emit_indent (fz_context *ctx, emitter *e)
{
  for (unsigned i= 0; i < e->indent_level * 4; ++i)
    fz_putc (ctx, e->out, ' ');
}

static void
emit_array (fz_context *ctx, emitter *e, pdfout_data *data);

static void
emit_title (fz_context *ctx, fz_output *out, pdfout_data *title)
{
  int len;

  char *title_str = pdfout_data_scalar_get(ctx, title, &len);

  if (memchr (title_str, '\n', len) == NULL)
    {
      fz_puts(ctx, out, title_str);
      return;
    }

  fz_warn (ctx, "title with newlines: '%s', replacing it with spaces",
	   title_str);
  char *copy = fz_strdup(ctx, title_str);
  for (int i = 0; i < strlen(copy); ++i) {
    if (copy[i] == '\n')
      copy[i] = ' ';
  }
  fz_puts (ctx, out, copy);
  free (copy);
}

static void
emit_hash (fz_context *ctx, emitter *e, pdfout_data *hash)
{
  pdfout_data *title = pdfout_data_hash_gets (ctx, hash, "title");
  pdfout_data *page = pdfout_data_hash_gets (ctx, hash, "page");
  int len;
  emit_indent (ctx, e);
  emit_title (ctx, e->out, title);
  
  fz_puts (ctx, e->out, " ");
  fz_puts (ctx, e->out, pdfout_data_scalar_get (ctx, page, &len));
  fz_puts (ctx, e->out, "\n");

  pdfout_data *kids = pdfout_data_hash_gets (ctx, hash, "kids");

  if (kids)
    {
      e->indent_level++;
      emit_array (ctx, e, kids);
      e->indent_level--;
    }
  
}

static void
emit_array (fz_context *ctx, emitter *e, pdfout_data *data)
{
  int len = pdfout_data_array_len(ctx, data);
  for (int i = 0; i < len; ++i)
    emit_hash (ctx, e, pdfout_data_array_get (ctx, data, i));
      
}

static void
emit (fz_context *ctx, pdfout_emitter *emit, pdfout_data *data)
{
  emitter *e = (emitter *) emit;
  if (e->finished)
    pdfout_throw (ctx, "finished outline wysiwyg emitter called");
  e->finished = true;

  emit_array(ctx, e, data);
}

static void
emitter_drop(fz_context *ctx, pdfout_emitter *emitter)
{
  free (emitter);
}

pdfout_emitter *
pdfout_emitter_outline_wysiwyg_new (fz_context *ctx, fz_output *stm)
{
  emitter *result = fz_malloc_struct (ctx, emitter);
  result->super.drop = emitter_drop;
  result->super.emit = emit;
  result->out = stm;
  return &result->super;
}


typedef struct {
  pdfout_parser super;
  fz_stream *stream;
  bool finished;

  
  /* Used during get_lines.  */
  int line_number;
  int previous_indent_level;
  int difference;		/* Set by 'd=...' */
  
} parser;


static bool
line_is_ws (fz_context *ctx, const char *line, int len)
{
  for (int i = 0; i < len; ++i)
    if (pdfout_isspace (line[i]) == false)
      return false;
  return true;
}

static bool
line_is_diff_setter (fz_context *ctx, const char *line, int len, parser *parse)
{
  const char *p = line;
  while (pdfout_isspace(*p))
    ++p;
  if (*p++ != 'd')
    return false;
  if (*p++ != '=')
    return false;
  const char *number = p;
  if (*p == '-')
    ++p;

  while (pdfout_isdigit(*p))
    ++p;

  const char *tail = p;
  if (tail == number)
    /* No digits.  */
    return false;
  while (pdfout_isspace(*p))
    ++p;
  if (p - line != len)
    /* Tail contains non-whitespace.  */
    return false;

  /* Success!  */
  parse->difference = pdfout_strtoint(ctx, number, NULL);
  return true;
}

static int
get_indent_level (fz_context *ctx, const char *p)
{
  int spaces = 0;
  while (*p++ == ' ')
    ++spaces;
  return spaces / 4;
}

static void
parse_error (fz_context *ctx, const char *msg, const char *line, int len,
	     parser *p)
{
  pdfout_throw (ctx, "In line %d: %s:\n%.*s", p->line_number, msg, len, line);
}

static void
data_hash_push_int (fz_context *ctx, pdfout_data *hash, const char *key, int x)
{
  pdfout_data *key_data = pdfout_data_scalar_new (ctx, key, strlen (key));
  char buf[100];
  int len = pdfout_snprintf(ctx, buf, "%d", x);
  pdfout_data *value = pdfout_data_scalar_new (ctx, buf, len);
  pdfout_data_hash_push (ctx, hash, key_data, value);
}

static void
data_hash_push_string_key (fz_context *ctx, pdfout_data *hash, const char *key,
			   pdfout_data *value)
{
  pdfout_data *key_data = pdfout_data_scalar_new (ctx, key, strlen (key));
  pdfout_data_hash_push (ctx, hash, key_data, value);
}
  
static void
add_line (fz_context *ctx, pdfout_data *lines, fz_buffer *line_buf, parser *p)
{
  ++p->line_number;
  char *line;
  int len = fz_buffer_storage (ctx, line_buf, (unsigned char **) &line);

  if (line_is_ws (ctx, line, len))
    return;

  if (line_is_diff_setter (ctx, line, len, p))
    return;

  if (len < 3)
    parse_error (ctx, "invalid line", line, len, p);
  
  const char *title = line;
  int indent_level = get_indent_level (ctx, title);
  title += indent_level * 4;

  /* Read trailing whitespace, number, separator.  */
  const char *trailing_ws = line + len - 1;
  while (pdfout_isspace(*trailing_ws))
    --trailing_ws;
  ++trailing_ws;

  const char *number = trailing_ws - 1;
  while (pdfout_isdigit(*number))
    --number;

  if (trailing_ws - number < 2)
    parse_error (ctx, "no number", line, len, p);
  
  if (*number == '-')
    --number;
  ++number;

  const char *separator = number - 1;
  while ((pdfout_isspace(*separator) || *separator == '.'))
    {
      if (separator == title)
	parse_error (ctx, "no title", line, len, p);
      --separator;
    }
  ++separator;

  if (separator == number)
    parse_error (ctx, "no separator between title and page number", line, len,
		 p);

  /* Check indent level.   */
  if (p->line_number == 1 && indent_level != 0)
    parse_error (ctx, "first line must not have indentation", line, len, p);

  if (indent_level > p->previous_indent_level + 1)
    parse_error (ctx, "too mutch indentation", line, len, p);

  p->previous_indent_level = indent_level;

  /* Success.  */
  
  pdfout_data *hash = pdfout_data_hash_new(ctx);
  pdfout_data_hash_push_key_value (ctx, hash, "title", title,
				   separator - title);
  fz_try (ctx)
  {
    /* Throws on overflow.  */
    int page = pdfout_strtoint (ctx, number, NULL);
    page += p->difference;
    data_hash_push_int (ctx, hash, "page", page);
    
    data_hash_push_int (ctx, hash, "level", indent_level);

    pdfout_data_array_push (ctx, lines, hash);
  }
  fz_catch (ctx)
  {
    pdfout_data_drop (ctx, hash);
    fz_rethrow (ctx);
  }
}

static pdfout_data *
get_lines (fz_context *ctx, parser *p)
{
  fz_buffer *line_buf = NULL;
  pdfout_data *lines = pdfout_data_array_new (ctx);

  fz_try (ctx)
  {
    while (pdfout_getline (ctx, &line_buf, p->stream) != -1)
      add_line (ctx, lines, line_buf, p);
  }
  fz_always (ctx)
  {
    fz_drop_buffer (ctx, line_buf);
  }
  fz_catch (ctx)
  {
    pdfout_data_drop (ctx, lines);
    fz_rethrow (ctx);
  }
  return lines;
}

static void
add_title_token (fz_context *ctx, pdfout_data *tokens, pdfout_data *line_hash)
{
  pdfout_data *token = pdfout_data_copy (ctx, line_hash);
  const char *type = "title";
  pdfout_data_hash_push_key_value (ctx, token, "type", type, strlen (type));
  pdfout_data_array_push (ctx, tokens, token);
}

static void
add_eof_token (fz_context *ctx, pdfout_data *tokens)
{
  pdfout_data *token = pdfout_data_hash_new (ctx);
  pdfout_data_hash_push_key_value (ctx, token, "type", "eof", strlen ("eof"));
  pdfout_data_array_push (ctx, tokens, token);
}

static void
add_kids_token (fz_context *ctx, pdfout_data *tokens, const char *type)
{
  pdfout_data *token = pdfout_data_hash_new (ctx);
  pdfout_data_hash_push_key_value (ctx, token, "type", type, strlen (type));
  pdfout_data_array_push (ctx, tokens, token);
}

static void
add_kids_start_token (fz_context *ctx, pdfout_data *tokens)
{
  add_kids_token (ctx, tokens, "kids-start");
}

static void
add_kids_end_token (fz_context *ctx, pdfout_data *tokens)
{
  add_kids_token (ctx, tokens, "kids-end");
}



static int
data_hash_key_to_int (fz_context *ctx, pdfout_data *hash, const char *key)
{
  pdfout_data *scalar = pdfout_data_hash_gets (ctx, hash, key);
  int len;
  const char *str = pdfout_data_scalar_get (ctx, scalar, &len);
  return pdfout_strtoint_null (ctx, str);
}

static pdfout_data *get_tokens (fz_context *ctx, pdfout_data *lines)
{
  pdfout_data *tokens = pdfout_data_array_new (ctx);
  int num = pdfout_data_array_len (ctx, lines);

  int previous_level = 0;

  /* Surround with start/end tokens to make it simpler to parse.  */
  add_kids_start_token (ctx, tokens);
  
  pdfout_data *line = pdfout_data_array_get (ctx, lines, 0);
  add_title_token (ctx, tokens, line);
  
  for (int i = 1; i < num; ++i) {
     line = pdfout_data_array_get (ctx, lines, i);
     int level = data_hash_key_to_int (ctx, line, "level");

     if (level > previous_level)
       add_kids_start_token (ctx, tokens);
     else if (level < previous_level)
       for (int j = 0; j < previous_level - level; ++j)
	 add_kids_end_token (ctx, tokens);
     
     add_title_token (ctx, tokens, line);
     previous_level = level;
  }
  
  for (int i = 0; i < previous_level; ++i)
    add_kids_end_token (ctx, tokens);

  add_kids_end_token (ctx, tokens);
  add_eof_token (ctx, tokens);
  
  return tokens;
}

typedef enum {
  TITLE_TOKEN,
  KIDS_START_TOKEN,
  KIDS_END_TOKEN,
  EOF_TOKEN,
} token_type;

typedef struct {
  pdfout_data *tokens;
  int read;
  token_type lah;
} token_parser;

static bool
eq(const char *a, const char *b)
{
  return strcmp(a, b) == false;
}

static void
token_parser_read (fz_context *ctx, token_parser *parser)
{
  pdfout_data *token = pdfout_data_array_get (ctx, parser->tokens,
					      parser->read);
  ++parser->read;
  pdfout_data *type = pdfout_data_hash_gets (ctx, token, "type");
  int len;
  const char *type_str = pdfout_data_scalar_get (ctx, type, &len);
  parser->lah = eq (type_str, "title") ? TITLE_TOKEN :
    eq (type_str, "kids-start") ? KIDS_START_TOKEN :
    eq (type_str, "kids-end") ? KIDS_END_TOKEN :
    EOF_TOKEN;
}

static token_parser *
token_parser_new (fz_context *ctx, pdfout_data *tokens)
{
  token_parser *result = fz_malloc_struct (ctx, token_parser);
  result->tokens = tokens;
  token_parser_read (ctx, result);
  return result;
}

static void
token_parser_drop (fz_context *ctx, token_parser *parser)
{
  free (parser);
}

static void
token_parse_terminal (fz_context *ctx, token_parser *parser, token_type tok)
{
  if (parser->lah != tok)
    pdfout_throw (ctx, "got token: %d, expected: %d", parser->lah, tok);
  
  token_parser_read (ctx, parser);
}

static pdfout_data *
token_parse_kids_array (fz_context *ctx, token_parser *parser);
  
static pdfout_data *
token_parse_title (fz_context *ctx, token_parser *parser)
{
  token_parse_terminal (ctx, parser, TITLE_TOKEN);
  pdfout_data *tokens = parser->tokens;
  
  pdfout_data *token = pdfout_data_array_get (ctx, tokens, parser->read - 2);
  pdfout_data *title = pdfout_data_hash_gets (ctx, token, "title");
  pdfout_data *page = pdfout_data_hash_gets (ctx, token, "page");
  pdfout_data *outline = pdfout_data_hash_new (ctx);
  pdfout_data *title_copy = pdfout_data_copy (ctx, title);
  pdfout_data *page_copy = pdfout_data_copy (ctx, page);
  
  data_hash_push_string_key (ctx, outline, "title", title_copy);
  data_hash_push_string_key (ctx, outline, "page", page_copy);

  if (parser->lah == KIDS_START_TOKEN)
    {
      pdfout_data *kids = token_parse_kids_array (ctx, parser);
      data_hash_push_string_key (ctx, outline, "kids", kids);
    }

  return outline;
}

static pdfout_data *
token_parse_kids_array (fz_context *ctx, token_parser *parser)
{
  pdfout_data *kids = pdfout_data_array_new (ctx);
  token_parse_terminal (ctx, parser, KIDS_START_TOKEN);
  while (parser->lah == TITLE_TOKEN)
    {
      pdfout_data *kid = token_parse_title (ctx, parser);
      pdfout_data_array_push (ctx, kids, kid);
    }
  token_parse_terminal (ctx, parser, KIDS_END_TOKEN);
  return kids;
}

static pdfout_data *
token_parse (fz_context *ctx, pdfout_data *tokens)
{
  token_parser *parser = token_parser_new (ctx, tokens);
  pdfout_data *outline = token_parse_kids_array (ctx, parser);
  assert (parser->lah == EOF_TOKEN);
  token_parser_drop (ctx, parser);
  return outline;
}

static pdfout_data *
parser_parse (fz_context *ctx, pdfout_parser *parse)
{
  parser *p = (parser *) parse;

  if (p->finished)
    pdfout_throw (ctx, "call to finished outline wysiwyg parser");
  p->finished = true;
  
  pdfout_data *lines = get_lines (ctx, p);
  if (pdfout_data_array_len (ctx, lines) == 0)
    return lines;
  
  pdfout_data *tokens = get_tokens (ctx, lines);
  pdfout_data *outline = token_parse(ctx, tokens);
  
  pdfout_data_drop (ctx, tokens);
  pdfout_data_drop (ctx, lines);

  return outline;
}

static void
parser_drop(fz_context *ctx, pdfout_parser *parser)
{
  free (parser);
}

pdfout_parser *
pdfout_parser_outline_wysiwyg_new (fz_context *ctx, fz_stream *stm)
{
  parser *result = fz_malloc_struct (ctx, parser);
  result->super.drop = parser_drop;
  result->super.parse = parser_parse;
  result->stream = fz_keep_stream (ctx, stm);
  return &result->super;
}
