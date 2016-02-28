#ifndef PDFOUT_HAVE_JSON_H
#define PDFOUT_HAVE_JSON_H

/* Scanner.  */

typedef struct json_scanner_s json_scanner;

json_scanner *json_scanner_new (fz_context *ctx, fz_stream *stream);
void json_scanner_drop (fz_context *ctx, json_scanner *scanner);

char *json_scanner_value (fz_context *ctx, json_scanner *scanner);

int json_scanner_value_len (fz_context *ctx, json_scanner *scanner);
typedef enum json_token_e {
  JSON_TOK_INVALID = -1,
  JSON_TOK_EOF = 1,
  
  JSON_TOK_STRING,	        /* "..."  */
  JSON_TOK_NUMBER,		/* [ minus ] int [ frac ] [ exp ]  */
  JSON_TOK_FALSE,		/* false  */
  JSON_TOK_NULL,		/* null  */
  JSON_TOK_TRUE,		/* true  */

  JSON_TOK_BEGIN_ARRAY,		
  JSON_TOK_END_ARRAY,		

  JSON_TOK_BEGIN_OBJECT, 
  JSON_TOK_END_OBJECT,		

  JSON_TOK_VALUE_SEPARATOR,     

  JSON_TOK_NAME_SEPARATOR,
  
} json_token;

json_token json_scanner_scan (fz_context *ctx, json_scanner *scanner);


/* Parser.  */

typedef struct json_parser_s json_parser;

void json_parser_drop (fz_context *ctx, json_parser *parser);

json_parser *json_parser_new (fz_context *ctx, fz_stream *stm);


json_token json_parser_parse (fz_context *ctx, json_parser *parser,
			      char **value, int *value_len);
  



#endif	/* PDFOUT_HAVE_JSON_H */
