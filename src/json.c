#include "common.h"
#include "data.h"
#include "c-ctype.h"
#include "unistr.h"
#include "uniwidth.h"

typedef struct scanner_s {
  fz_stream *stream;
  int lookahead;

  /* value of string/number token.  */
  fz_buffer *value;
  
  /* Current line. Used for error message.  */
  int line_count;
  
} scanner;

typedef enum token_e {
  TOK_EOF = -1,
  				          
  TOK_STRING = 1,		/* "..."  */
  TOK_NUMBER,       /* [ minus ] int [ frac ] [ exp ]  */
  TOK_FALSE,         /* false  */
  TOK_NULL,           /* null  */
  TOK_TRUE,           /* true  */

  TOK_BEGIN_ARRAY,
  TOK_END_ARRAY,
  
  TOK_BEGIN_OBJECT,
  TOK_END_OBJECT,

  TOK_VALUE_SEPARATOR,

  TOK_NAME_SEPARATOR,
  
} token;

static void scanner_read (fz_context *ctx, scanner *scanner)
{
  if (scanner->lookahead == '\n')
    ++scanner->line_count;
  
  scanner->lookahead = fz_read_byte (ctx, scanner->stream);
}

static void
scanner_drop (fz_context *ctx, scanner *scanner)
{
  fz_drop_stream (ctx, scanner->stream);
  fz_drop_buffer (ctx, scanner->value);
  free (scanner);
}


static scanner *
scanner_new (fz_context *ctx, fz_stream *stream)
{
  scanner *result;
  
  fz_var (result);
  fz_try (ctx)
  {
    result = fz_malloc_struct (ctx, scanner);
  
    result->stream = fz_keep_stream (ctx, stream);

    result->value = fz_new_buffer (ctx, 1);
  
    result->line_count = 1;
    scanner_read (ctx, result);
  }
  fz_catch (ctx)
  {
    scanner_drop (ctx, result);
  }
  
  return result;
}

static void PDFOUT_PRINTFLIKE (3)
scanner_error (fz_context *ctx, scanner *scanner, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  pdfout_msg ("Error In line %d:", scanner->line_count);
  if (fmt)
    pdfout_vmsg (fmt, ap);
  else
    pdfout_msg ("Syntax error");

  fz_throw (ctx, FZ_ERROR_GENERIC, "Scanner error");
}


static token
scanner_scan_literal (fz_context *ctx, const char *lit, scanner *scanner)
{
  const char *p = lit;
  while (*p)
    {
      if (scanner->lookahead != *p++)
	scanner_error (ctx, scanner, "Error in literal '%s'", lit);
      scanner_read(ctx, scanner);
    }
  switch (lit[0])
    {
    case 'f':
      return TOK_FALSE;
    case 'n':
      return TOK_NULL;
    case 't':
      return TOK_TRUE;
    default:
      abort();
    }
}



static void push_byte (fz_context *ctx, scanner *scanner, int byte)
{
  fz_write_buffer_byte (ctx, scanner->value, byte);	
  scanner_read (ctx, scanner);			
}

static void zero_term (fz_context *ctx, fz_buffer *buf)
{
  fz_write_buffer_byte (ctx, buf, 0);
  --buf->len;
}


static const char *json_check_number (const char *number, int len)
{

  if (len && *number == '.')
    return "Leading point in Number";
    
   /* Optional minus.  */
  if (len && *number == '-')
    ++number, --len;

  /* Mandatory int.  */
  if (len && *number == '0')
    {
      ++number, --len;
      if (len && c_isdigit (*number))
	return "Leading zero in number";
    }
  else if (len && *number >= '1' && *number <= '9')
    {
      ++number, --len;
      while (len && c_isdigit (*number))
	++number, --len;
    }
  else
      return "Not a valid JSON number";
  
   /* Optional frac.  */
  if (len && *number == '.')
    {
      ++number, --len;
      if (len && c_isdigit (*number))
	{
	  while (len && c_isdigit (*number))
	    ++number, --len;
	}
      else
	return "No digit in fraction part";
    }

  /* Optional exp.  */
  if (len && c_tolower (*number) == 'e')
    {
      ++number, --len;
      if (len && (*number == '-' || *number == '+'))
	++number, --len;
      if (len && c_isdigit (*number))
	{
	  while (len && c_isdigit (*number))
	    ++number, --len;
	}
      else
	return "Incomplete exponential notation";
    }

  if (len != 0)
    return "Not a valid JSON Number";
  
  return NULL;
}

static token
scanner_scan_number (fz_context *ctx, scanner *scanner)
{
  scanner->value->len = 0;
  char c;
  while (c = scanner->lookahead, c_isdigit (c) || c == 'e' || c == 'E'
	 || c == '-' || c == '+' || c == '.')
    push_byte (ctx, scanner, scanner->lookahead);

  char *num = (char *) scanner->value->data;
  int value_len = scanner->value->len;
  const char *error = json_check_number (num, value_len);
  if (error)
    scanner_error (ctx, scanner, "Invalid number '%.*s': %s", value_len, num,
		   error);

  zero_term (ctx, scanner->value);
  
  return TOK_NUMBER;
}


/* Return math value of two char hex sequence.  */
static int get_hex_sequence (fz_context *ctx, scanner *scanner)
{
  if (scanner->lookahead != 'u')
    scanner_error (ctx, scanner, "Invalid hex escape");
  scanner_read (ctx, scanner);

  int retval = 0;
  for (int i = 0; i < 4; ++i)
    {
      int lah = c_tolower (scanner->lookahead);
      if (c_isxdigit (lah))
	{
	  retval = retval * 16 + (c_isdigit(lah) ? lah - '0' : lah - 'a' + 10);
	  scanner_read (ctx, scanner);
	}
      else
	scanner_error (ctx, scanner, "Invalid hex digit '%c'.",
		       scanner->lookahead);
    }
  return retval;
}
  
static void utf16_escape_sequence (fz_context *ctx, scanner *scanner)
{
  int leading = get_hex_sequence (ctx, scanner);
  
  int uc;
  if (leading <= 0xD7FF || leading >= 0xE000)
    {
      uc = leading;
    }
  else
    {
      if (leading >= 0xDA00)
	scanner_error (ctx, scanner, "Trailing surrogate at start of\
 UTF-16 surrogate pair.");
      
      /* We have a leading surrogate.  */
      if (scanner->lookahead != '\\')
	scanner_error (ctx, scanner, "Leading surrogate not followed by \
trailing surrogate.");
      scanner_read (ctx, scanner);
      int trailing = get_hex_sequence (ctx, scanner);
      if (trailing < 0xDA00 || trailing > 0xDFFF)
	scanner_error (ctx, scanner, "Expected trailing UTF-16 surrogate.");
      
      uc = (((leading & 0x3FF) + 0x40) << 10) + (trailing & 0x3FF);
    }
  
  uint8_t buf[4];
  int len = u8_uctomb (buf, uc, 4);
  if (len < 1)
    {
      pdfout_msg ("invalid codepoint: U+%x", uc);
      abort();
    }
  fz_write_buffer (ctx, scanner->value, buf, len);
}

static void escape_sequence (fz_context *ctx, scanner *scanner)
{
  scanner_read (ctx, scanner);
  switch (scanner->lookahead)
    {
    case '"': case '\\': case '/':
      push_byte (ctx, scanner, scanner->lookahead); break;
    case 'b': push_byte (ctx, scanner, '\b'); break;
    case 'f': push_byte (ctx, scanner, '\f'); break;
    case 'n': push_byte (ctx, scanner, '\n'); break;
    case 'r': push_byte (ctx, scanner, '\r'); break;
    case 't': push_byte (ctx, scanner, '\t'); break;
    case 'u':
      utf16_escape_sequence (ctx, scanner); break;
    default:
      scanner_error (ctx, scanner, "Invalid escape escape char: %c",
		     scanner->lookahead);
    }
}

static void
utf8_sequence (fz_context *ctx, scanner *scanner)
{
  fz_buffer *value = scanner->value;
  int start_len = value->len;
  char *data = (char *) value->data;
  while (1)
    {
      int lah = scanner->lookahead;
      
      if (lah <= 0x1f || lah == '\\' || lah == '"')
	{
	  char *problem = pdfout_check_utf8 (data + start_len, value->len
					     - start_len);
	  if (problem)
	    scanner_error (ctx, scanner, "Invalid UTF-8.");
	  
	  break;
	}
      fz_write_buffer_byte (ctx, value, lah);
      scanner_read (ctx, scanner);
    }
}

static token
scanner_scan_string (fz_context *ctx, scanner *scanner)
{
  assert (scanner->lookahead == '"');
  scanner->value->len = 0;
  
  scanner_read (ctx, scanner);
  while (1)
    {
      int lah = scanner->lookahead;
      if (lah == '"')
	{
	  scanner_read (ctx, scanner);
	  break;
	}
      else if (lah == '\\')
	escape_sequence (ctx, scanner);
      else if (lah >= 0 && lah <= 0x1f)
	scanner_error (ctx, scanner,
		       "Character 0x%02x has to be escaped in string.", lah);
      else if (lah == EOF)
	scanner_error (ctx, scanner, "Unfinished string.");
      else
	utf8_sequence (ctx, scanner);
    }

  zero_term (ctx, scanner->value);
  
  return TOK_STRING;
}

#define is_ws(c)  (c == ' ' || c == '\t' || c == '\n' || c == '\r')

static void skip_ws (fz_context *ctx, scanner *scanner)
{
  while (is_ws (scanner->lookahead))
    scanner_read (ctx, scanner);
}

#define return_token(tok)                                       \
  do {scanner_read (ctx, scanner); return tok;} while (0)

static token
scanner_scan (fz_context *ctx, scanner *scanner)
{
  skip_ws (ctx, scanner);

  switch (scanner->lookahead)
    {
    case EOF: return_token (TOK_EOF);
    case '"': return scanner_scan_string (ctx, scanner);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '-': case '.': return scanner_scan_number (ctx, scanner);
    case 'f': return scanner_scan_literal (ctx, "false", scanner);
    case 'n': return scanner_scan_literal (ctx, "null", scanner);
    case 't': return scanner_scan_literal (ctx, "true", scanner);
    case '[': return_token (TOK_BEGIN_ARRAY);
    case ']': return_token (TOK_END_ARRAY);
    case '{': return_token (TOK_BEGIN_OBJECT);
    case '}': return_token (TOK_END_OBJECT);
    case ',': return_token (TOK_VALUE_SEPARATOR);
    case ':': return_token (TOK_NAME_SEPARATOR);
    default:
      scanner_error (ctx, scanner, NULL);
    }
  abort ();
}


typedef struct {
  pdfout_parser super;
  
  scanner *scanner;
  
  token lookahead;

  bool finished;

} json_parser;

static void parser_drop (fz_context *ctx, json_parser *parser)
{
  scanner_drop (ctx, parser->scanner);
  free (parser);
}

static void
json_parser_drop (fz_context *ctx, pdfout_parser *parser)
{
  parser_drop (ctx, (json_parser *) parser);
}


static void parser_read (fz_context *ctx, json_parser *parser)
{
  parser->lookahead = scanner_scan (ctx, parser->scanner);
}

/* 
   Just the usual recursive decent LL(1) parser.

   Terminals: string, number, false, null, true, [, ], {, }, ',', :
   
   Nonterminals: S, value, array, value_list, separated_value_list,
   object, pair_list, separated_pair_list
   
   empty string: ε
   
   Grammar:

   S -> value

   value -> array | object | string | number | false | null | true

   array -> [ value_list ]
   value_list -> ε | value separated_value_list
   separated_value_list -> ε | , value separated_value_list

   object -> { pair_list }
   pair_list -> ε | string : value separated_pair_list
   separated_pair_list -> ε | , pair separated_pair_list
   

   FIRST sets

   First(S) = First(value) = [ | { | string | number | false | null | true
   First(array) = [
   First(value_list) = ε | First(value)
   First(separated_value_list) = ε | ,
   First(object) = {
   First(pair_list) = ε | string
   First(separated_pair_list) = ε | ,

   Follow sets (as needed)
   
   Follow(value_list) = ]
   Follow(separated_value_list) = ]
   Follow(pair_list) = }
   Follow(separated_pair_list) = }

*/

static void PDFOUT_NORETURN
parser_error (fz_context *ctx, json_parser *parser)
{
  scanner *scanner = parser->scanner;
  parser->finished = true;

  fz_throw (ctx, FZ_ERROR_GENERIC, "Syntax error in line %d",
	    scanner->line_count);
}

static void parse_terminal (fz_context *ctx, json_parser *parser, token tok)
{
  if (parser->lookahead != tok)
    parser_error (ctx, parser);
  parser_read (ctx, parser);
}

static bool parser_accept (fz_context *ctx, json_parser *parser, token tok)
{
  if (parser->lookahead != tok)
    return false;

  parser_read (ctx, parser);
  return true;
}

static pdfout_data *
parse_string (fz_context *ctx, json_parser *parser, token tok)
{
  parse_terminal (ctx, parser, tok);
  fz_buffer *buf = parser->scanner->value;
  return pdfout_data_scalar_new (ctx, (char *) buf->data, buf->len);
}

static pdfout_data *
parse_literal (fz_context *ctx, json_parser *parser, token tok)
{
  parse_terminal (ctx, parser, tok);
  const char *s;
  switch (tok)
    {
    case TOK_FALSE: s = "false"; break;
    case TOK_NULL: s = "null"; break;
    case TOK_TRUE: s = "true"; break;
    default: abort ();
    }
  
  return pdfout_data_scalar_new (ctx, s, strlen (s));
}

static pdfout_data *parse_value (fz_context *ctx, json_parser *parser);

static pdfout_data *parse_array (fz_context *ctx, json_parser *parser)
{
  parse_terminal (ctx, parser, TOK_BEGIN_ARRAY);
  pdfout_data *array = pdfout_data_array_new (ctx);
  fz_try (ctx)
  {
    if (parser_accept (ctx, parser, TOK_END_ARRAY))
      break;
    
    pdfout_data *value;
    do
      {
	value = parse_value (ctx, parser);
	pdfout_data_array_push (ctx, array, value);
      } while (parser_accept (ctx, parser, TOK_VALUE_SEPARATOR));
    
    parse_terminal (ctx, parser, TOK_END_ARRAY);
  }
  fz_catch (ctx)
  {
    pdfout_data_drop (ctx, array);
    fz_rethrow (ctx);
  }
  return array;
}

static pdfout_data *parse_hash (fz_context *ctx, json_parser *parser)
{
  parse_terminal (ctx, parser, TOK_BEGIN_OBJECT);
  pdfout_data *hash = pdfout_data_hash_new (ctx);
  fz_try (ctx)
  {
    if (parser_accept (ctx, parser, TOK_END_OBJECT))
      return hash;

    pdfout_data *key, *value;
    do
      {
	key = parse_string (ctx, parser, TOK_STRING);
	parse_terminal (ctx, parser, TOK_NAME_SEPARATOR);
	value = parse_value (ctx, parser);
	pdfout_data_hash_push (ctx, hash, key, value);
      } while (parser_accept (ctx, parser, TOK_VALUE_SEPARATOR));

    parse_terminal (ctx, parser, TOK_END_OBJECT);
  }
  fz_catch (ctx)
  {
    pdfout_data_drop (ctx, hash);
    fz_rethrow (ctx);
  }
  return hash;
}

static pdfout_data *parse_value (fz_context *ctx, json_parser *parser)
{
  token tok = parser->lookahead;
  switch (tok)
    {
    case TOK_BEGIN_ARRAY: return parse_array (ctx, parser);
      
    case TOK_BEGIN_OBJECT: return parse_hash (ctx, parser);
      
    case TOK_STRING:
    case TOK_NUMBER: return parse_string (ctx, parser, tok);
      
    case TOK_FALSE:
    case TOK_NULL:
    case TOK_TRUE: return parse_literal (ctx, parser, tok);
    default:
      fz_warn (ctx, "unexpected token: %d", tok);
      parser_error (ctx, parser);
    }
}


static pdfout_data *
parser_parse (fz_context *ctx, pdfout_parser *parser)
{
  json_parser *p = (json_parser *) parser;
  if (p->finished)
    fz_throw (ctx, FZ_ERROR_GENERIC, "call to finished parser");

  p->finished = true;
  parser_read (ctx, p);
  pdfout_data *result = parse_value (ctx, p);

  fz_try (ctx)
  {
    parse_terminal (ctx, p, TOK_EOF);
  }
  fz_catch (ctx)
  {
    pdfout_data_drop (ctx, result);
    fz_rethrow (ctx);
  }
		      
  return result;
}

pdfout_parser *
pdfout_parser_json_new (fz_context *ctx, fz_stream *stm)
{
  json_parser *result;

  result = fz_malloc_struct (ctx, json_parser);
  result->super.drop = json_parser_drop;
  result->super.parse = parser_parse;
  fz_try (ctx)
  {
    result->scanner = scanner_new (ctx, stm);
  }
  fz_catch (ctx)
  {
    parser_drop (ctx, result);
    fz_rethrow (ctx);
  }
  
  return &result->super;
}

/* Emitter stuff. */

typedef struct json_emitter_s {
  pdfout_emitter super;
  
  fz_output *out;
  
  bool finished;

  unsigned indent;
  unsigned indent_level;
  
} json_emitter;

static void
emitter_drop (fz_context *ctx, pdfout_emitter *emitter)
{
  free (emitter);
}


static bool is_literal (const char *value, int len)
{
#define literal_equal(literal)				\
  (sizeof literal == len + 1 && memcmp (literal, value, len) == 0)

  if (literal_equal ("false") || literal_equal ("null")
      || literal_equal ("true"))
    return true;

  return false;
}

static void
pdfout_json_escape_string (fz_context *ctx, fz_output *out, const char *value,
			   int value_len)
{
  if (is_literal (value, value_len)
      || json_check_number (value, value_len) == NULL)
    {
      fz_write (ctx, out, value, value_len);
      return;
    }
  
  if (pdfout_check_utf8 (value, value_len))
    fz_throw (ctx, FZ_ERROR_GENERIC, "invalid UTF-8");
  fz_putc (ctx, out, '"');
  for (int i = 0; i < value_len; ++i)
    {
      unsigned char byte = value[i];
      if (byte < 0x20 || byte == '"' || byte == '\\')
	{
	  /* Needs escaping.  */
	  switch (byte)
	    {
	    case '"':  fz_puts (ctx, out, "\\\""); break;
	    case '\\': fz_puts (ctx, out, "\\\\"); break;
	    case '\b': fz_puts (ctx, out, "\\b");  break;
	    case '\f': fz_puts (ctx, out, "\\f");  break;
	    case '\n': fz_puts (ctx, out, "\\n");  break;
	    case '\r': fz_puts (ctx, out, "\\r");  break;
	    case '\t': fz_puts (ctx, out, "\\t");  break;
	    default:
	      {
		char seq[] = "\\u0000";
		fz_snprintf(seq + 4, 2, "%02x", byte);
		fz_write (ctx, out, seq, 6);
	      }
	    }
	}
      else
	/* No escaping needed.  */
	fz_putc (ctx, out, byte);
    }
  fz_putc (ctx, out, '"');
}
static void
emit_string (fz_context *ctx, json_emitter *emitter, pdfout_data *data)
{
  fz_output *out = emitter->out;
  int value_len;
  const char *value = pdfout_data_scalar_get (ctx, data, &value_len);
  pdfout_json_escape_string (ctx, out, value, value_len);
}

static void emit_indent (fz_context *ctx, json_emitter *emitter)
{
  for (unsigned i= 0; i < emitter->indent * emitter->indent_level; ++i)
    fz_putc (ctx, emitter->out, ' ');
}

static void emit_value_separator (fz_context *ctx, json_emitter *emitter)
{
  fz_puts (ctx, emitter->out, ",\n");
  emit_indent (ctx, emitter);
}

static void
emit_value (fz_context *ctx, json_emitter *emitter, pdfout_data *data);

  
static void
emit_array (fz_context *ctx, json_emitter *emitter, pdfout_data *data)
{
  fz_output *out = emitter->out;
  fz_puts (ctx, out, "[");
  int len = pdfout_data_array_len (ctx, data);
  if (len == 0)
    {
      fz_puts (ctx, out, "]");
      return;
    }
  fz_puts (ctx, out, "\n");
  ++emitter->indent_level;
  emit_indent (ctx, emitter);

  for (int i = 0; i < len; ++i)
    {
      pdfout_data *item = pdfout_data_array_get (ctx, data, i);
      emit_value (ctx, emitter, item);
      if (i < len - 1)
	emit_value_separator (ctx, emitter);
    }
  fz_puts (ctx, out ,"\n");
  --emitter->indent_level;
  emit_indent (ctx, emitter);
  fz_puts (ctx, out, "]");
}

static void
emit_hash (fz_context *ctx, json_emitter *emitter, pdfout_data *data)
{
  fz_output *out = emitter->out;
  fz_puts (ctx, out, "{");
  int len = pdfout_data_hash_len (ctx, data);
  if (len == 0)
    {
      fz_puts (ctx, out, "}");
      return;
    }

  fz_puts (ctx, out, "\n");
  ++emitter->indent_level;
  emit_indent (ctx, emitter);
  
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *key = pdfout_data_hash_get_key (ctx, data, i);
      emit_value (ctx, emitter, key);
      fz_puts (ctx, out, ": ");
      pdfout_data *value = pdfout_data_hash_get_value (ctx, data, i);
      emit_value (ctx, emitter, value);
      if (i < len - 1)
	emit_value_separator (ctx, emitter);
    }
  fz_puts (ctx, out, "\n");
  emitter->indent_level--;
  emit_indent (ctx, emitter);
  fz_puts (ctx, out, "}");

}

static void
emit_value (fz_context *ctx, json_emitter *emitter, pdfout_data *data)
{
  if (pdfout_data_is_scalar (ctx, data))
    emit_string (ctx, emitter, data);
  else if (pdfout_data_is_array (ctx, data))
    emit_array (ctx, emitter, data);
  else if (pdfout_data_is_hash (ctx, data))
    emit_hash (ctx, emitter, data);
}

static void
emitter_emit (fz_context *ctx, pdfout_emitter *emitter, pdfout_data *data)
{
  json_emitter *e = (json_emitter *) emitter;
  if (e->finished)
    fz_throw (ctx, FZ_ERROR_GENERIC, "Call to finished JSON emitter");

  e->finished = true;
  
  emit_value (ctx, e, data);

  fz_puts (ctx, e->out, "\n");
}
  

pdfout_emitter *
pdfout_emitter_json_new (fz_context *ctx, fz_output *stm)
{
  json_emitter *result = fz_malloc_struct (ctx, json_emitter);
  
  result->super.drop = emitter_drop;
  result->super.emit = emitter_emit;
  
  result->out = stm;

  result->indent = 4;
  result->indent_level = 0;
  
  return &result->super;
}
