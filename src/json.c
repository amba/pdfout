#include "common.h"
#include "json.h"
#include "c-ctype.h"
#include "unistr.h"
#include "uniwidth.h"

typedef struct json_scanner_s {
  fz_stream *stream;
  int lookahead;

  /* value of string/number token.  */
  fz_buffer *value;
  
  /* Current line. Used for error message.  */
  fz_buffer *line;
  int line_count;
  
  /* Current error position in LINE. Used to draw a '^' at the position of the
     error. If greater than the line's length, the '^' will be drawn one
     column beyond the line's end.  */
  int error;

  /* Position in LINE of the last successfully scanned token. Used for the
     parsers error messages.  */
  int token_start;
} json_scanner;

typedef enum scanner_token_e {
  JSON_TOK_INVALID = JSON_INVALID,
  JSON_TOK_EOF = JSON_EOF,
  				          
  JSON_TOK_STRING = JSON_STRING,       /* "..."  */
  JSON_TOK_NUMBER = JSON_NUMBER,       /* [ minus ] int [ frac ] [ exp ]  */
  JSON_TOK_FALSE = JSON_FALSE,         /* false  */
  JSON_TOK_NULL = JSON_NULL,           /* null  */
  JSON_TOK_TRUE = JSON_TRUE,           /* true  */

  JSON_TOK_BEGIN_ARRAY = JSON_BEGIN_ARRAY,
  JSON_TOK_END_ARRAY = JSON_END_ARRAY,
  
  JSON_TOK_BEGIN_OBJECT = JSON_BEGIN_OBJECT,
  JSON_TOK_END_OBJECT = JSON_END_OBJECT,

  JSON_TOK_VALUE_SEPARATOR,

  JSON_TOK_NAME_SEPARATOR,
  
} scanner_token;


static void scanner_read (fz_context *ctx, json_scanner *scanner)
{
  if (scanner->lookahead == '\n')
    {
      ++scanner->line_count;
      scanner->line->len = 0;
    }
  
  scanner->lookahead = fz_read_byte (ctx, scanner->stream);
  if (scanner->lookahead != EOF && scanner->lookahead != '\n')
    {
      fz_write_buffer_byte (ctx, scanner->line, scanner->lookahead);
      ++scanner->error;
    }
}

static void
json_scanner_drop (fz_context *ctx, json_scanner *scanner)
{
  fz_drop_stream (ctx, scanner->stream);
  fz_drop_buffer (ctx, scanner->value);
  fz_drop_buffer (ctx, scanner->line);
  free (scanner);
}


static json_scanner *
json_scanner_new (fz_context *ctx, fz_stream *stream)
{
  json_scanner *result;
  
  fz_var (result);
  fz_try (ctx)
  {
    result = fz_malloc_struct (ctx, json_scanner);
  
    result->stream = fz_keep_stream (ctx, stream);

    result->value = fz_new_buffer (ctx, 1);
  
    result->line = fz_new_buffer (ctx, 1);
  
    result->line_count = 1;
    result->error = 0;
    scanner_read (ctx, result);
  }
  fz_catch (ctx)
  {
    json_scanner_drop (ctx, result);
  }
  
  return result;
}


/* static char * */
/* json_scanner_value (fz_context *ctx, json_scanner *scanner) */
/* { */
/*   return (char *) scanner->value->data; */
/* } */

/* static int */
/* json_scanner_value_len (fz_context *ctx, json_scanner *scanner) */
/* { */
/*   return scanner->value->len; */
/* } */

static void error_marker (json_scanner *scanner, int error)
{
  uint8_t *text = scanner->line->data;
  int len = scanner->line->len;
  int width = u8_width (text, error > len ? len : error, "UTF-8");
  if (error > len)
    ++width;
  pdfout_msg_raw (" %.*s\n", len, text);
  for (int i = 0; i < width; ++i)
    pdfout_msg_raw (" ");
  pdfout_msg_raw ("^\n");
}

static scanner_token  PDFOUT_WUR PDFOUT_PRINTFLIKE (3)
scanner_error (fz_context *ctx, json_scanner *scanner, const char *fmt, ...)
{
  va_list ap;
  va_start (ap, fmt);

  int error_save = scanner->error;
  /* Finish to read the current line.  */
  while (scanner->lookahead != '\n' && scanner->lookahead != EOF)
    scanner_read (ctx, scanner);
  pdfout_msg ("In line %d:", scanner->line_count);
  if (fmt)
    pdfout_vmsg (fmt, ap);
  else
    pdfout_msg ("Syntax error:");
  
  error_marker (scanner, error_save);
  return JSON_TOK_INVALID;
}


static scanner_token
scanner_scan_literal (fz_context *ctx, const char *lit, json_scanner *scanner)
{
  const char *p = lit;
  while (*p)
    {
      if (scanner->lookahead != *p++)
	return scanner_error (ctx, scanner, "Error in literal '%s'", lit);
      scanner_read(ctx, scanner);
    }
  switch (lit[0])
    {
    case 'f':
      return JSON_TOK_FALSE;
    case 'n':
      return JSON_TOK_NULL;
    case 't':
      return JSON_TOK_TRUE;
    default:
      abort();
    }
}



static void push_byte (fz_context *ctx, json_scanner *scanner, int byte)
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
  
  return 0;
}

static scanner_token
scanner_scan_number (fz_context *ctx, json_scanner *scanner)
{
  scanner->value->len = 0;
  char c;
  while (c = scanner->lookahead, c_isdigit (c) || c == 'e' || c == 'E'
	 || c == '-' || c == '+' || c == '.')
    push_byte (ctx, scanner, scanner->lookahead);

  const char *error = json_check_number ((char *) scanner->value->data,
					 scanner->value->len);
  if (error)
    return scanner_error (ctx, scanner, "%s", error);

  zero_term (ctx, scanner->value);
  
  return JSON_TOK_NUMBER;
}


/* Return -1 on error.  */
static int get_hex_sequence (fz_context *ctx, json_scanner *scanner)
{
  if (scanner->lookahead != 'u')
    return -1;
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
	{
	  pdfout_msg ("Invalid hex digit '%c'.", scanner->lookahead);
	  return -1;
	}
    }
  return retval;
}
  
/* Return non-zero on invalid sequence.  */
static int utf16_escape_sequence (fz_context *ctx, json_scanner *scanner)
{
  int leading = get_hex_sequence (ctx, scanner);
  if (leading < 0)
    return 1;
  
  int uc;
  if (leading <= 0xD7FF || leading >= 0xE000)
    {
      uc = leading;
    }
  else
    {
      if (leading >= 0xDA00)
	{
	  pdfout_msg ("Trailing surrogate at start of UTF-16 surrogate pair.");
	  return 1;
	}

      /* We have a leading surrogate.  */
      if (scanner->lookahead != '\\')
	{
	  pdfout_msg ("Leading surrogate not followed by trailing surrogate.");
	  return 1;
	}
      scanner_read (ctx, scanner);
      int trailing = get_hex_sequence (ctx, scanner);
      if (trailing < 0)
	return 1;
      if (trailing < 0xDA00 || trailing > 0xDFFF)
	{
	  pdfout_msg ("Expected trailing UTF-16 surrogate.");
	  return 1;
	}
      
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
  
  return 0;
}

/* Return non-zero on invalid sequence.  */
static int escape_sequence (fz_context *ctx, json_scanner *scanner)
{
  int ret;
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
	ret = utf16_escape_sequence (ctx, scanner);
	if (ret)
	  scanner->error -= 6;
	return ret;
    default:
      --scanner->error;
      return 1;
    }
  return 0;
}

static scanner_token
utf8_sequence (fz_context *ctx, json_scanner *scanner)
{
  fz_buffer *value = scanner->value;
  int start_len = value->len;
  char *data = (char *) value->data;
  char *start = data + start_len;
  int start_line_len = scanner->line->len;
  while (1)
    {
      int lah = scanner->lookahead;
      
      if (lah <= 0x1f || lah == '\\' || lah == '"')
	{
	  char *problem = pdfout_check_utf8 (data + start_len, value->len
					     - start_len);
	  if (problem)
	    {
	      scanner->error = start_line_len + (problem - start);
	      return scanner_error (ctx, scanner, "Invalid UTF-8.");
	    }
	  break;
	}
      fz_write_buffer_byte (ctx, value, lah);
      scanner_read (ctx, scanner);
    }
  return 0;
}

static scanner_token
scanner_scan_string (fz_context *ctx, json_scanner *scanner)
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
	{
	  if (escape_sequence (ctx, scanner))
	    return scanner_error (ctx, scanner, "Invalid escape sequence.");
	}
      else if (lah >= 0 && lah <= 0x1f)
	return
	  scanner_error (ctx, scanner,
			 "Character 0x%02x has to be escaped in string.", lah);
      else if (lah == EOF)
	return scanner_error (ctx, scanner, "Unfinished string.");
      else
	{
	  scanner_token ret = utf8_sequence (ctx, scanner);
	  if (ret)
	    return ret;
	}
    }

  zero_term (ctx, scanner->value);
  
  return JSON_TOK_STRING;
}

#define is_ws(c)  (c == ' ' || c == '\t' || c == '\n' || c == '\r')

static void skip_ws (fz_context *ctx, json_scanner *scanner)
{
  while (is_ws (scanner->lookahead))
    scanner_read (ctx, scanner);
}

#define return_token(tok)					\
  do {scanner_read (ctx, scanner); return tok;} while (0)

static scanner_token
json_scanner_scan (fz_context *ctx, json_scanner *scanner)
{
  skip_ws (ctx, scanner);

  scanner->token_start = scanner->line->len;
  
  switch (scanner->lookahead)
    {
    case EOF: return_token (JSON_TOK_EOF);
    case '"': return scanner_scan_string (ctx, scanner);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '-': case '.': return scanner_scan_number (ctx, scanner);
    case 'f': return scanner_scan_literal (ctx, "false", scanner);
    case 'n': return scanner_scan_literal (ctx, "null", scanner);
    case 't': return scanner_scan_literal (ctx, "true", scanner);
    case '[': return_token (JSON_TOK_BEGIN_ARRAY);
    case ']': return_token (JSON_TOK_END_ARRAY);
    case '{': return_token (JSON_TOK_BEGIN_OBJECT);
    case '}': return_token (JSON_TOK_END_OBJECT);
    case ',': return_token (JSON_TOK_VALUE_SEPARATOR);
    case ':': return_token (JSON_TOK_NAME_SEPARATOR);
    default:
      return scanner_error (ctx, scanner, NULL);
    }
}

typedef struct json_parser_s {
  json_scanner *scanner;
  
  /* Parser stack.  */
  fz_buffer *stack;
  
  scanner_token lookahead;

  
  bool reached_eof;

  /* Did we already return a JSON_INVALID?  */
  bool failed;
  
} json_parser;

static void stack_push  (fz_context *ctx, fz_buffer *stack, int value)
  
{
  fz_write_buffer (ctx, stack, &value, sizeof (int));
}

static int stack_pop (fz_context *ctx, fz_buffer *stack)
{
  assert (stack->len % sizeof (int) == 0);
  stack->len -= sizeof (int);
  assert (stack->len >= 0);
  return ((int *) stack->data)[stack->len / sizeof(int)];
}

void
json_parser_drop (fz_context *ctx, json_parser *parser)
{
  fz_drop_buffer (ctx, parser->stack);
  json_scanner_drop (ctx, parser->scanner);
  free (parser);
}



typedef enum {
  STRING = JSON_TOK_STRING,
  NUMBER,
  FALSE,
  NUL,
  TRUE,
  BEGIN_ARRAY,
  END_ARRAY,
  BEGIN_OBJECT,
  END_OBJECT,
  VALUE_SEPARATOR,
  NAME_SEPARATOR,
  
  START = 1000,
  VALUE,
  
  ARRAY,
  VALUE_LIST,
  SEP_VALUE_LIST,
  
  OBJECT,
  PAIR_LIST,
  SEP_PAIR_LIST,
  PAIR,

  EMPTY
} symbol;

json_parser *
json_parser_new (fz_context *ctx, fz_stream *stm)
{
  json_parser *result;

  fz_var (result);
  
  fz_try (ctx)
  {
    result = fz_malloc_struct (ctx, json_parser);
    result->scanner = json_scanner_new (ctx, stm);
    result->stack = fz_new_buffer (ctx, 64);
    stack_push (ctx, result->stack, JSON_TOK_EOF);
    stack_push (ctx, result->stack, START);
  }
  fz_catch (ctx)
  {
    json_parser_drop (ctx, result);
    fz_rethrow (ctx);
  }

  return result;
}


/* 
   For the algorithm, see the section on non-recursive LL(1)-Parsing in
   the Dragonbook or <www.cs.purdue.edu/homes/xyzhang/spring11/notes/ll.pdf>.

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
#define ISTERM(sym) (sym < START)

static scanner_token parser_read (fz_context *ctx, json_parser *parser)
{
  scanner_token result = json_scanner_scan (ctx, parser->scanner);
  parser->lookahead = result;
  if (result == JSON_TOK_INVALID)
    parser->failed = true;
  return result;
}

static json_token parser_error (fz_context *ctx, json_parser *parser)
{
  json_scanner *scanner = parser->scanner;
  pdfout_msg ("In line %d:", scanner->error);
  pdfout_msg ("Syntax error:");
  error_marker (scanner, scanner->token_start);
  parser->failed = true;
  return JSON_INVALID;
}

#define DEF(NONTERM, TERM, args...)\
  [NONTERM - START][TERM - STRING] = (int[5]) { args }

static const int *const parse_table[9][11] = {
  DEF (START, BEGIN_ARRAY, VALUE),
  DEF (START, BEGIN_OBJECT, VALUE),
  DEF (START, STRING, VALUE),
  DEF (START, NUMBER, VALUE),
  DEF (START, FALSE, VALUE),
  DEF (START, NUL, VALUE),
  DEF (START, TRUE, VALUE),

  DEF (VALUE, BEGIN_ARRAY, ARRAY),
  DEF (VALUE, BEGIN_OBJECT, OBJECT),
  DEF (VALUE, STRING, STRING),
  DEF (VALUE, NUMBER, NUMBER),
  DEF (VALUE, FALSE, FALSE),
  DEF (VALUE, NUL, NUL),
  DEF (VALUE, TRUE, TRUE),

  DEF (ARRAY, BEGIN_ARRAY, BEGIN_ARRAY, VALUE_LIST, END_ARRAY),

  DEF (VALUE_LIST, BEGIN_ARRAY, ARRAY, SEP_VALUE_LIST),
  DEF (VALUE_LIST, END_ARRAY, EMPTY),
  DEF (VALUE_LIST, BEGIN_OBJECT, OBJECT, SEP_VALUE_LIST),
  DEF (VALUE_LIST, STRING, STRING, SEP_VALUE_LIST),
  DEF (VALUE_LIST, NUMBER, NUMBER, SEP_VALUE_LIST),
  DEF (VALUE_LIST, FALSE, FALSE, SEP_VALUE_LIST),
  DEF (VALUE_LIST, NUL, NUL, SEP_VALUE_LIST),
  DEF (VALUE_LIST, TRUE, TRUE, SEP_VALUE_LIST),

  DEF (SEP_VALUE_LIST, END_ARRAY, EMPTY),
  DEF (SEP_VALUE_LIST, VALUE_SEPARATOR, VALUE_SEPARATOR, VALUE,
       SEP_VALUE_LIST),

  DEF (OBJECT, BEGIN_OBJECT, BEGIN_OBJECT, PAIR_LIST, END_OBJECT),

  DEF (PAIR_LIST, END_OBJECT, EMPTY),
  DEF (PAIR_LIST, STRING, STRING, NAME_SEPARATOR, VALUE, SEP_PAIR_LIST),

  DEF (SEP_PAIR_LIST, END_OBJECT, EMPTY),
  DEF (SEP_PAIR_LIST, VALUE_SEPARATOR, VALUE_SEPARATOR, STRING, NAME_SEPARATOR,
       VALUE, SEP_PAIR_LIST),
};


json_token
json_parser_parse (fz_context *ctx, json_parser *parser, char **value,
		   int *value_len)
{
  if (parser->failed)
    {
      fz_warn (ctx, "calling json_parser_parse after failure");
      return JSON_INVALID;
    }
  
  if (parser->reached_eof)
    {
      fz_warn (ctx, "calling json_parser_parse after reaching EOF");
      return JSON_INVALID;
    }
  
  if (parser_read (ctx, parser) < 0)
    return JSON_INVALID;
  
  while (1)
    {
      symbol X = stack_pop (ctx, parser->stack);
      scanner_token t = parser->lookahead;
      
      if (ISTERM (X))
	{
	  if (X == (symbol) t)
	    {
	      switch (t)
		{
		case JSON_TOK_EOF:
		  parser->reached_eof = true;
		  return JSON_EOF;
		case JSON_TOK_VALUE_SEPARATOR:
		case JSON_TOK_NAME_SEPARATOR:
		  /* In those cases nothing is returned. Get the next token
		     and continue parsing.  */
		  if (parser_read (ctx, parser) < 0)
		    return JSON_INVALID;
		  continue;
		case JSON_TOK_STRING: case JSON_TOK_NUMBER:
		  /* We need to return a value.  */
		  if (value)
		    {
		      *value = (char *) parser->scanner->value->data;
		      *value_len = parser->scanner->value->len;
		    }
		  return t;
		default:
		  return t;
		}
	    }
	  else
	    return parser_error (ctx, parser);
	}

      /* Nontermial. Make table lookup.  */
      if (t - JSON_TOK_STRING < 0)
	{
	  ++parser->scanner->token_start;
	  return parser_error (ctx, parser);
	}
      
      const int *production = parse_table[X - START][t - JSON_TOK_STRING];
      if (production == NULL)
	return parser_error (ctx, parser);
      else if (production[0] == EMPTY)
	{
	  /* Do nothing.  */
	}
      else
	for (int i = 4; i >= 0; --i)
	  if (production[i])
	    stack_push (ctx, parser->stack, production[i]);
      
    } /* end of while (1) */
}

/* Emitter stuff. */

typedef struct json_emitter_s {
  
  fz_output *out;
  fz_buffer *stack;
  
  bool reached_eof;
  
  bool failed;

  unsigned indent;

  unsigned indent_level;
  
} json_emitter;

void
json_emitter_drop (fz_context *ctx, json_emitter *emitter)
{
  fz_drop_buffer (ctx, emitter->stack);
  free (emitter);
}

json_emitter *
json_emitter_new (fz_context *ctx, fz_output *stm)
{
  json_emitter *result;

  fz_var (result);
  
  fz_try (ctx)
  {
    result = fz_malloc_struct (ctx, json_emitter);

    result->stack = fz_new_buffer (ctx, 64);

    result->out = stm;

    result->indent = 4;

    result->indent_level = 0;
    
    stack_push (ctx, result->stack, JSON_TOK_EOF);
    stack_push (ctx, result->stack, START);
  }
  fz_catch (ctx)
  {
    json_emitter_drop (ctx, result);
    fz_rethrow (ctx);
  }

  return result;
}

void
json_emitter_set_indent (fz_context *ctx, json_emitter *emitter,
			 unsigned indent)
{
  emitter->indent = indent;
}

static void emit_string (fz_context *ctx, fz_output *out,
			 const char *value, int value_len)
{
  if (pdfout_check_utf8 (value, value_len))
    fz_throw (ctx, FZ_ERROR_GENERIC, "invalid UTF-8 in json_emitter_emit");
  fz_putc (ctx, out, '"');
  for (int i = 0; i < value_len; ++i)
    {
      unsigned char byte = value[i];
      if (byte < 0x20 || byte == '"' || byte == '\\')
	{
	  /* Needs escaping.  */
	  switch (byte)
	    {
	    case '"':
	      fz_puts (ctx, out, "\\\"");
	      break;
	    case '\\':
	      fz_puts (ctx, out, "\\\\");
	      break;
	    case '\b':
	      fz_puts (ctx, out, "\\b");
	      break;
	    case '\f':
	      fz_puts (ctx, out, "\\f");
	      break;
	    case '\n':
	      fz_puts (ctx, out, "\\n");
	      break;
	    case '\r':
	      fz_puts (ctx, out, "\\r");
	      break;
	    case '\t':
	      fz_puts (ctx, out, "\\t");
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

static void emit_indent (fz_context *ctx, json_emitter *emitter)
{
  for (unsigned i= 0; i < emitter->indent * emitter->indent_level; ++i)
    fz_putc (ctx, emitter->out, ' ');
}
      

static void emit_event (fz_context *ctx, json_emitter *emitter, symbol s,
			const char *value, int value_len)
{
  fz_output *out = emitter->out;
  const char *error;
  switch (s)
    {
    case STRING:
      emit_string (ctx, out, value, value_len);
      break;
    case NUMBER:
      error = json_check_number (value, value_len);
      if (error)
	fz_throw (ctx, FZ_ERROR_GENERIC, "%s", error);
      fz_write (ctx, out, value, value_len);
      break;
    case FALSE:
      fz_puts (ctx, out, "false");
      break;
    case NUL:
      fz_puts (ctx, out, "null");
      break;
    case TRUE:
      fz_puts (ctx, out, "true");
      break;
    case BEGIN_ARRAY:
      fz_puts (ctx, out, "[\n");
      ++emitter->indent_level;
      emit_indent (ctx, emitter);
      break;
    case END_ARRAY:
      --emitter->indent_level;
      fz_putc (ctx, out, '\n');
      emit_indent (ctx, emitter);
      fz_putc (ctx, out, ']');
      break;
    case BEGIN_OBJECT:
      fz_puts (ctx, out, "{\n");
      ++emitter->indent_level;
      emit_indent (ctx, emitter);
      break;
    case END_OBJECT:
      --emitter->indent_level;
      fz_putc (ctx, out, '\n');
      emit_indent (ctx, emitter);
      fz_putc (ctx, out, '}');
      break;
    case NAME_SEPARATOR:
      fz_puts (ctx, out, ": ");
      break;
    case VALUE_SEPARATOR:
      fz_puts (ctx, out, ",\n");
      emit_indent (ctx, emitter);
      break;
    default:
      abort ();
    }
      
}
   
static const int *const emitter_parse_table[9][9] = {
  DEF (START, BEGIN_ARRAY, VALUE),
  DEF (START, BEGIN_OBJECT, VALUE),
  DEF (START, STRING, VALUE),
  DEF (START, NUMBER, VALUE),
  DEF (START, FALSE, VALUE),
  DEF (START, NUL, VALUE),
  DEF (START, TRUE, VALUE),

  DEF (VALUE, BEGIN_ARRAY, ARRAY),
  DEF (VALUE, BEGIN_OBJECT, OBJECT),
  DEF (VALUE, STRING, STRING),
  DEF (VALUE, NUMBER, NUMBER),
  DEF (VALUE, FALSE, FALSE),
  DEF (VALUE, NUL, NUL),
  DEF (VALUE, TRUE, TRUE),

  DEF (ARRAY, BEGIN_ARRAY, BEGIN_ARRAY, VALUE_LIST, END_ARRAY),

  DEF (VALUE_LIST, BEGIN_ARRAY, ARRAY, SEP_VALUE_LIST),
  DEF (VALUE_LIST, END_ARRAY, EMPTY),
  DEF (VALUE_LIST, BEGIN_OBJECT, OBJECT, SEP_VALUE_LIST),
  DEF (VALUE_LIST, STRING, STRING, SEP_VALUE_LIST),
  DEF (VALUE_LIST, NUMBER, NUMBER, SEP_VALUE_LIST),
  DEF (VALUE_LIST, FALSE, FALSE, SEP_VALUE_LIST),
  DEF (VALUE_LIST, NUL, NUL, SEP_VALUE_LIST),
  DEF (VALUE_LIST, TRUE, TRUE, SEP_VALUE_LIST),

  /* Insert VALUE_SEPARATOR and NAME_SEPARATOR into output.  */

  DEF (SEP_VALUE_LIST, BEGIN_ARRAY, VALUE_SEPARATOR, ARRAY, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, END_ARRAY, EMPTY),
  DEF (SEP_VALUE_LIST, BEGIN_OBJECT, VALUE_SEPARATOR, OBJECT, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, STRING, VALUE_SEPARATOR, STRING, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, NUMBER, VALUE_SEPARATOR, NUMBER, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, FALSE, VALUE_SEPARATOR, FALSE, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, NUL, VALUE_SEPARATOR, NUL, SEP_VALUE_LIST),
  DEF (SEP_VALUE_LIST, TRUE, VALUE_SEPARATOR, TRUE, SEP_VALUE_LIST),
  
  DEF (OBJECT, BEGIN_OBJECT, BEGIN_OBJECT, PAIR_LIST, END_OBJECT),

  DEF (PAIR_LIST, END_OBJECT, EMPTY),
  DEF (PAIR_LIST, STRING, STRING, NAME_SEPARATOR, VALUE, SEP_PAIR_LIST),

  DEF (SEP_PAIR_LIST, END_OBJECT, EMPTY),
  DEF (SEP_PAIR_LIST, STRING, VALUE_SEPARATOR, STRING, NAME_SEPARATOR,
       VALUE, SEP_PAIR_LIST),
};

static int emitter_error (fz_context *ctx, json_emitter *emitter, json_token t)
{
  /* FIXME: name tokens */
  fz_warn (ctx, "json emitter: unexpected token %d", t);
  emitter->failed = true;
  return -1;
}

int
json_emitter_emit (fz_context *ctx, json_emitter *emitter, json_token t,
		   const char *value, int value_len)
{
  if (emitter->failed)
    return emitter_error (ctx, emitter, t);
  
  if (emitter->reached_eof)
    return emitter_error (ctx, emitter, t);
  
  while (1)
    {
      symbol X = stack_pop (ctx, emitter->stack);
      
      if (ISTERM (X))
	{
	  if (X == NAME_SEPARATOR || X == VALUE_SEPARATOR)
	    {
	      emit_event (ctx, emitter, X, NULL, 0);
	      continue;
	    }
	  else if (X == (symbol) t)
	    {
	      if (t == JSON_EOF)
		{
		  emitter->reached_eof = true;
		  fz_putc (ctx, emitter->out, '\n');
/*      * * * * * * * * * ****  * * * *  */
/*      FIXME: assert stack length == 0  */
/*  * * * * * * *** * * * * * * *  */
		}
	      else 
		emit_event (ctx, emitter, t, value, value_len);
	      
	      return 0;
	    }
	  else
	    return emitter_error (ctx, emitter, t);
	}
	  
      /* Nontermial. Make table lookup.  */
      if (t - JSON_TOK_STRING < 0)
	return emitter_error (ctx, emitter, t);
      
      const int *production =
	emitter_parse_table[X - START][t - JSON_TOK_STRING];
      
      if (production == NULL)
	{
	  fz_warn (ctx, "did not find production in json_emitter_emit");
	  return emitter_error (ctx, emitter, t);
	}
      else if (production[0] == EMPTY)
	{
	  /* Do nothing.  */
	}
      else
	for (int i = 4; i >= 0; --i)
	  if (production[i])
	    stack_push (ctx, emitter->stack, production[i]);
      
    } /* end of while (1) */
}

