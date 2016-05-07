#ifndef PDFOUT_DATA_H
#define PDFOUT_DATA_H

typedef struct pdfout_data_s pdfout_data;

bool pdfout_data_is_scalar (fz_context *ctx, pdfout_data *data);
bool pdfout_data_is_array (fz_context *ctx, pdfout_data *data);
bool pdfout_data_is_hash (fz_context *ctx, pdfout_data *data);

pdfout_data *pdfout_data_create_scalar (fz_context *ctx, const char *value,
					int len);
pdfout_data *pdfout_data_create_array (fz_context *ctx);
pdfout_data *pdfout_data_create_hash (fz_context *ctx);

/* recursively drop.  */
void pdfout_data_drop (fz_context *ctx, pdfout_data *data);

char *pdfout_data_scalar_get (fz_context *ctx, pdfout_data *scalar, int *len);



int pdfout_data_array_len (fz_context *ctx, pdfout_data *array);

void pdfout_data_array_push (fz_context *ctx, pdfout_data *array,
			     pdfout_data *entry);

pdfout_data *pdfout_data_array_get (fz_context *ctx, pdfout_data *array,
				    int pos);



int pdfout_data_hash_len (fz_context *ctx, pdfout_data *hash);

void pdfout_data_hash_push (fz_context *ctx, pdfout_data *hash,
			    pdfout_data *key, pdfout_data *value);

pdfout_data *pdfout_data_hash_get_key (fz_context *ctx, pdfout_data *hash,
				       int pos);
pdfout_data *pdfout_data_hash_get_value (fz_context *ctx, pdfout_data *hash,
					 int pos);

/* key must be null-terminated.  */
pdfout_data *pdfout_data_hash_gets (fz_context *ctx, pdfout_data *hash,
				    char *key);

#endif	/* PDFOUT_DATA_H */
