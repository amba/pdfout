#include "common.h"
#include "data.h"
#include "c-ctype.h"
#include "unistr.h"
#include "uniwidth.h"

typedef struct json_scanner_s {
  fz_stream *stream;
  int lookahead;

  /* value of string/number token.  */
  fz_buffer *value;
  
  /* Current line. Used for error message.  */
  int line_count;
  
} json_scanner;

typedef enum json_scanner_token_e {
  JSON_TOK_EOF,
  				          
  JSON_TOK_STRING,       /* "..."  */
  JSON_TOK_NUMBER,       /* [ minus ] int [ frac ] [ exp ]  */
  JSON_TOK_FALSE,         /* false  */
  JSON_TOK_NULL,           /* null  */
  JSON_TOK_TRUE,           /* true  */

  JSON_TOK_BEGIN_ARRAY,
  JSON_TOK_END_ARRAY,
  
  JSON_TOK_BEGIN_OBJECT,
  JSON_TOK_END_OBJECT,

  JSON_TOK_VALUE_SEPARATOR,

  JSON_TOK_NAME_SEPARATOR,
  
} json_scanner_token;

static void scanner_read (fz_context *ctx, json_scanner *scanner)
{
  if (scanner->lookahead == '\n')
    ++scanner->line_count;
  
  scanner->lookahead = fz_read_byte (ctx, scanner->stream);
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

static void PDFOUT_PRINTFLIKE (3)
scanner_error (fz_context *ctx, json_scanner *scanner, const char *fmt, ...)
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


static scanner_token
scanner_scan_literal (fz_context *ctx, const char *lit, json_scanner *scanner)
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
    scanner_error (ctx, scanner, "%s", error);

  zero_term (ctx, scanner->value);
  
  return JSON_TOK_NUMBER;
}


/* Return math value of two char hex sequence.  */
static int get_hex_sequence (fz_context *ctx, json_scanner *scanner)
{
  if (scanner->lookahead != 'u')
    scanner_error (ctx, "Invalid hex escape");
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
	scanner_error (ctx, "Invalid hex digit '%c'.", scanner->lookahead);
    }
  return retval;
}
  
static void utf16_escape_sequence (fz_context *ctx, json_scanner *scanner)
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
	scanner_error (ctx,
		       "Trailing surrogate at start of UTF-16 surrogate pair.");
      
      /* We have a leading surrogate.  */
      if (scanner->lookahead != '\\')
	scanner_error (ctx,
		       "Leading surrogate not followed by trailing surrogate.");
      scanner_read (ctx, scanner);
      int trailing = get_hex_sequence (ctx, scanner);
      if (trailing < 0)
	return 1;
      if (trailing < 0xDA00 || trailing > 0xDFFF)
	scanner_error (ctx, "Expected trailing UTF-16 surrogate.");
      
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

static void escape_sequence (fz_context *ctx, json_scanner *scanner)
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
      utf16_escape_sequence (ctx, scanner);
    default:
      scanner_error (ctx, "Invalid escape sequence");
    }
  return 0;
}

static void
utf8_sequence (fz_context *ctx, json_scanner *scanner)
{
  fz_buffer *value = scanner->value;
  int start_len = value->len;
  char *data = (char *) value->data;
  char *start = data + start_len;
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
  
  return JSON_TOK_STRING;
}

#define is_ws(c)  (c == ' ' || c == '\t' || c == '\n' || c == '\r')

static void skip_ws (fz_context *ctx, json_scanner *scanner)
{
  while (is_ws (scanner->lookahead))
    scanner_read (ctx, scanner);
}

static scanner_token
json_scanner_scan (fz_context *ctx, json_scanner *scanner)
{
  skip_ws (ctx, scanner);

  scanner->token_start = scanner->line->len;
  json_token tok;
  switch (scanner->lookahead)
    {
    case EOF: tok = JSON_TOK_EOF; break;
    case '"': tok = scanner_scan_string (ctx, scanner); break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '-': case '.': tok = scanner_scan_number (ctx, scanner); break;
    case 'f': tok = scanner_scan_literal (ctx, "false", scanner); break;
    case 'n': tok = scanner_scan_literal (ctx, "null", scanner); break;
    case 't': tok = scanner_scan_literal (ctx, "true", scanner); break;
    case '[': tok = JSON_TOK_BEGIN_ARRAY; break;
    case ']': tok = JSON_TOK_END_ARRAY; break;
    case '{': tok = JSON_TOK_BEGIN_OBJECT; break;
    case '}': tok = JSON_TOK_END_OBJECT; break;
    case ',': tok = JSON_TOK_VALUE_SEPARATOR; break;
    case ':': tok = JSON_TOK_NAME_SEPARATOR; break;
    default:
      scanner_error (ctx, scanner, NULL);
    }
  return tok;
}


typedef struct {
  pdfout_parser super;
  
  json_scanner *scanner;
  
  scanner_token lookahead;

} json_parser;

static void
parser_drop (fz_context *ctx, pdfout_parser *parser)
{
  json_scanner_drop (ctx, parser->scanner);
  free (parser);
}

pdfout_parser *
pdfout_parser_json_new (fz_context *ctx, fz_stream *stm);
{
  json_parser *result;

  result = fz_malloc_struct (ctx, json_parser);
  result->super.drop = parser_drop;
  result->super.parse = parser_parse;
  fz_try (ctx)
  {
    result->scanner = json_scanner_new (ctx, stm);
  }
  fz_catch (ctx)
  {
    json_parser_drop (ctx, result);
    fz_rethrow (ctx);
  }
  
  return &result->super;
}

static void parser_read (fz_context *ctx, json_parser *parser)
{
  parser->lookahead = json_scanner_scan (ctx, parser->scanner);
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

