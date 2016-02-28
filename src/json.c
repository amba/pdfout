#include "common.h"
#include "json.h"
#include "c-ctype.h"
#include "unistr.h"

typedef struct json_scanner_s {
  fz_stream *stream;
  int lookahead;

  /* value of string/number token.  */
  fz_buffer *value;
  
  /* Current line. Used for error message.  */
  fz_buffer *line;
  int line_count;
} json_scanner;

static void scanner_read (fz_context *ctx, json_scanner *scanner)
{
  if (scanner->lookahead == '\n')
    {
      ++scanner->line_count;
      scanner->line->len = 0;
    }
  
  scanner->lookahead = fz_read_byte (ctx, scanner->stream);
  if (scanner->lookahead != EOF && scanner->lookahead != '\n')
    fz_write_buffer_byte (ctx, scanner->line, scanner->lookahead);
}

void
json_scanner_drop (fz_context *ctx, json_scanner *scanner)
{
  fz_drop_stream (ctx, scanner->stream);
  fz_drop_buffer (ctx, scanner->value);
  fz_drop_buffer (ctx, scanner->line);
  free (scanner);
}


json_scanner *
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
    scanner_read (ctx, result);
  }
  fz_catch (ctx)
  {
    json_scanner_drop (ctx, result);
  }
  
  return result;
}


char *
json_scanner_value (fz_context *ctx, json_scanner *scanner)
{
  return (char *) scanner->value->data;
}

int
json_scanner_value_len (fz_context *ctx, json_scanner *scanner)
{
  return scanner->value->len;
}

static json_token PDFOUT_WUR
scanner_verror (fz_context *ctx, json_scanner *scanner, const char *fmt,
		va_list ap)
  
{
  int lah = scanner->lookahead;
  int column = scanner->line->len;
  
  /* Finish to read the current line.  */
  while (scanner->lookahead != '\n' && scanner->lookahead != EOF)
    scanner_read (ctx, scanner);
  
  pdfout_msg ("In input line %d, column %d: %.*s", scanner->line_count,
	      column, scanner->line->len, (char *) scanner->line->data);
  if (fmt)
    pdfout_vmsg(fmt, ap);
  else
    pdfout_msg ("Unexpected character '%c'", lah);
  return JSON_TOK_INVALID;
}

static json_token PDFOUT_WUR
scanner_error (fz_context *ctx, json_scanner *scanner, const char *format, ...)
{
  va_list ap;
  va_start (ap, format);
  return scanner_verror (ctx, scanner, format, ap);
}

static json_token
scanner_scan_literal (fz_context *ctx, const char *lit, json_scanner *scanner)
{
  const char *p = lit;
  while (*p)
    {
      if (scanner->lookahead != *p++)
	return scanner_error (ctx, scanner, "Error in literal '%s'",
			      lit);
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

static bool is_ws (int c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static void skip_ws (fz_context *ctx, json_scanner *scanner)
{
  while (is_ws (scanner->lookahead))
    scanner_read (ctx, scanner);
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

static json_token
scanner_scan_number (fz_context *ctx, json_scanner *scanner)
{
  scanner->value->len = 0;

  /* Optional minus.  */
  if (scanner->lookahead == '-')
    push_byte (ctx, scanner, '-');

  /* Mandatory int.  */
  if (scanner->lookahead == '0')
    {
      push_byte (ctx, scanner, '0');
      if (c_isdigit (scanner->lookahead))
	return scanner_error (ctx, scanner, "Leading zero in number.");
    }
  else if (scanner->lookahead >= '1' && scanner->lookahead <= '9')
    {
      push_byte (ctx, scanner, scanner->lookahead);
      while (c_isdigit (scanner->lookahead))
	push_byte (ctx, scanner, scanner->lookahead);
    }
  else
    return scanner_error (ctx, scanner, "Not a number");
  
  /* Optional frac.  */
  if (scanner->lookahead == '.')
    {
      push_byte (ctx, scanner, '.');
      if (c_isdigit (scanner->lookahead))
	{
	  while (c_isdigit (scanner->lookahead))
	    push_byte (ctx, scanner, scanner->lookahead);
	}
      else
	return scanner_error (ctx, scanner, "No digit in fraction part.");
    }

  /* Optional exp.  */
  if (c_tolower (scanner->lookahead) == 'e')
    {
      push_byte (ctx, scanner, scanner->lookahead);
      if (scanner->lookahead == '-' || scanner->lookahead == '+')
	push_byte (ctx, scanner, scanner->lookahead);
      if (c_isdigit (scanner->lookahead))
	{
	  push_byte (ctx, scanner, scanner->lookahead);
	  while (c_isdigit (scanner->lookahead))
	    push_byte (ctx, scanner, scanner->lookahead);
	}
      else
	return scanner_error (ctx, scanner,
			      "Incomplete exponential notation.");
    }

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
	  return -1;
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
      return utf16_escape_sequence (ctx, scanner);
    default:
      return 1;
    }
  return 0;
}

static json_token
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
	  if (pdfout_check_utf8 ((char *) scanner->value->data,
				 scanner->value->len))
	    return scanner_error (ctx, scanner, "Invalid UTF-8.");
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
			 "Character 0x%0x has to be escaped in string.", lah);
      else if (lah == EOF)
	return scanner_error (ctx, scanner, "Unfinished string.");
      else
	{
	  fz_write_buffer_byte (ctx, scanner->value, lah);
	  scanner_read (ctx, scanner);
	}
    }

  zero_term (ctx, scanner->value);
  
  return JSON_TOK_STRING;
}

#define return_token(tok) \
  do {scanner_read (ctx, scanner); return tok;} while (0)

json_token
json_scanner_scan (fz_context *ctx, json_scanner *scanner)
{
  skip_ws (ctx, scanner);
  switch (scanner->lookahead)
    {
    case EOF: return_token (JSON_TOK_EOF);
    case '"': return scanner_scan_string (ctx, scanner);
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case '-': return scanner_scan_number (ctx, scanner);
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
  
  json_token lookahead;
  
} json_parser;

static void stack_push  (fz_context *ctx, json_parser *parser, int value)
  
{
  fz_write_buffer (ctx, parser->stack, &value, sizeof (int));
}

static int stack_pop (fz_context *ctx, json_parser *parser)
{
  assert (parser->stack->len % sizeof (int) == 0);
  parser->stack->len -= sizeof (int);
  assert (parser->stack->len >= 0);
  return ((int *) parser->stack->data)[parser->stack->len / sizeof(int)];
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
    stack_push (ctx, result, JSON_TOK_EOF);
    stack_push (ctx, result, START);
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
   the Dragonbook.

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

static json_token parser_read (fz_context *ctx, json_parser *parser)
{
  json_token result = json_scanner_scan (ctx, parser->scanner);
  parser->lookahead = result;
  return result;
}

static json_token parser_error (fz_context *ctx, json_parser *parser,
				const char *format, ... )
{
  va_list ap;
  va_start (ap, format);

  return scanner_verror (ctx, parser->scanner, format, ap);
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
  if (parser_read (ctx, parser) < 0)
    return JSON_TOK_INVALID;
  
  while (1)
    {
      symbol X = stack_pop (ctx, parser);
      json_token t = parser->lookahead;
      
      if (ISTERM (X))
	{
	  if (X == (symbol) t)
	    {
	      switch (t)
		{
		case JSON_TOK_VALUE_SEPARATOR:
		case JSON_TOK_NAME_SEPARATOR:
		  if (parser_read (ctx, parser) < 0)
		    return JSON_TOK_INVALID;
		  continue;
		case JSON_TOK_STRING: case JSON_TOK_NUMBER:
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
	    return parser_error (ctx, parser, NULL);
	}

      /* Nontermial. Make table lookup.  */
      const int *production = parse_table[X - START][t - JSON_TOK_STRING];
      if (production == NULL)
	return parser_error (ctx, parser, NULL);
      else if (production[0] == EMPTY)
	{
	  /* Do nothing */
	}
      else
	for (int i = 4; i >= 0; --i)
	  if (production[i])
	    stack_push (ctx, parser, production[i]);
      
    }
}
