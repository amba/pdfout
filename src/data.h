#ifndef PDFOUT_DATA_H
#define PDFOUT_DATA_H

typedef struct pdfout_data_s pdfout_data;

bool pdfout_data_is_scalar (fz_context *ctx, pdfout_data *data);
bool pdfout_data_is_array (fz_context *ctx, pdfout_data *data);
bool pdfout_data_is_hash (fz_context *ctx, pdfout_data *data);

/* void pdfout_data_assert_scalar (fz_context *ctx, pdfout_data *data); */
/* void pdfout_data_assert_array (fz_context *ctx, pdfout_data *data); */
/* void pdfout_data_assert_hash (fz_context *ctx, pdfout_data *data); */

pdfout_data *pdfout_data_scalar_new (fz_context *ctx, const char *value,
				     int len);
pdfout_data *pdfout_data_array_new (fz_context *ctx);
pdfout_data *pdfout_data_hash_new (fz_context *ctx);

/* recursively drop.  */
void pdfout_data_drop (fz_context *ctx, pdfout_data *data);

char *pdfout_data_scalar_get (fz_context *ctx, pdfout_data *scalar, int *len);

bool
pdfout_data_scalar_eq (fz_context *ctx, pdfout_data *scalar, const char *s);


pdf_obj *pdfout_data_scalar_to_pdf_name (fz_context *ctx, pdf_document *doc,
					 pdfout_data *scalar);

pdf_obj *pdfout_data_scalar_to_pdf_str (fz_context *ctx, pdf_document *doc,
					pdfout_data *scalar);

pdf_obj *pdfout_data_scalar_to_pdf_int (fz_context *ctx, pdf_document *doc,
					pdfout_data *scalar);
pdf_obj *pdfout_data_scalar_to_pdf_real (fz_context *ctx, pdf_document *doc,
					 pdfout_data *scalar);

pdfout_data *pdfout_data_scalar_from_pdf (fz_context *ctx, pdf_obj *obj);

			    
/* char *
pdfout_data_scalar_get_string (fz_context *ctx, pdfout_data *scalar);*/



int pdfout_data_array_len (fz_context *ctx, pdfout_data *array);

void pdfout_data_array_push (fz_context *ctx, pdfout_data *array,
			     pdfout_data *entry);

pdfout_data *pdfout_data_array_get (fz_context *ctx, pdfout_data *array,
				    int pos);



int pdfout_data_hash_len (fz_context *ctx, pdfout_data *hash);

void pdfout_data_hash_push (fz_context *ctx, pdfout_data *hash,
			    pdfout_data *key, pdfout_data *value);

pdfout_data *pdfout_data_hash_get_key (fz_context *ctx, pdfout_data *hash,
				       int i);
pdfout_data *pdfout_data_hash_get_value (fz_context *ctx, pdfout_data *hash,
					 int i);
void pdfout_data_hash_push_key_value (fz_context *ctx, pdfout_data *hash,
				      const char *key, const char *value,
				      int value_len);



/* key must be null-terminated.  */
pdfout_data *pdfout_data_hash_gets (fz_context *ctx, pdfout_data *hash,
				    const char *key);

void pdfout_data_hash_get_key_value (fz_context *ctx, pdfout_data *hash,
				     char **key, char **value, int *value_len,
				     int i);

int pdfout_data_cmp (fz_context *ctx, pdfout_data *x, pdfout_data *y);

/*  Parsers.  */
typedef struct pdfout_parser_s pdfout_parser;


typedef void (*parser_drop_fn) (fz_context *ctx, pdfout_parser *parser);
typedef pdfout_data *(*parser_parse_fn) (fz_context *ctx, pdfout_parser *parser);

struct pdfout_parser_s {
  parser_drop_fn drop;
  parser_parse_fn parse;
};

void pdfout_parser_drop (fz_context *ctx, pdfout_parser *parser);

/* Throw on error.  */
pdfout_data *pdfout_parser_parse (fz_context *ctx, pdfout_parser *parser);

pdfout_parser *pdfout_parser_json_new (fz_context *ctx, fz_stream *stm);

/* Emitters. */

typedef struct pdfout_emitter_s pdfout_emitter;


typedef void (*emitter_drop_fn) (fz_context *ctx, pdfout_emitter *emitter);
typedef void (*emitter_emit_fn) (fz_context *ctx, pdfout_emitter *emitter,
				 pdfout_data *data);

struct pdfout_emitter_s {
  emitter_drop_fn drop;
  emitter_emit_fn emit;
};



void pdfout_emitter_drop (fz_context *ctx, pdfout_emitter *emitter);

void pdfout_emitter_emit (fz_context *ctx, pdfout_emitter *emitter,
			  pdfout_data *data);

pdfout_emitter *pdfout_emitter_json_new (fz_context *ctx, fz_output *out);


#endif	/* PDFOUT_DATA_H */
