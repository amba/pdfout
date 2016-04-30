#ifndef PDFOUT_HAVE_JSON_H
#define PDFOUT_HAVE_JSON_H

typedef enum json_token_e {
  JSON_INVALID = -1,
  JSON_EOF = 1,
  JSON_STRING = 2,	        /* "..."  */
  JSON_NUMBER,		/* [ minus ] int [ frac ] [ exp ]  */
  JSON_FALSE,		/* false  */
  JSON_NULL,		/* null  */
  JSON_TRUE,		/* true  */

  JSON_BEGIN_ARRAY,		
  JSON_END_ARRAY,		

  JSON_BEGIN_OBJECT, 
  JSON_END_OBJECT,

} json_token;


typedef struct json_parser_s json_parser;

void json_parser_drop (fz_context *ctx, json_parser *parser);

json_parser *json_parser_new (fz_context *ctx, fz_stream *stm);


json_token json_parser_parse (fz_context *ctx, json_parser *parser,
			      char **value, int *value_len);


#endif	/* PDFOUT_HAVE_JSON_H */
