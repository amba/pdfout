#include "common.h"
#include "data.h"
#include "c-ctype.h"
#include "unistr.h"
#include "uniwidth.h"

enum data_type {
  SCALAR,
  ARRAY,
  HASH
};

struct pdfout_data_s {
  enum data_type type;
};

typedef struct data_scalar_s {
  pdfout_data super;
  int len;
  char *value;
} data_scalar;

typedef struct data_array_s {
  pdfout_data super;
  int len, cap;
  pdfout_data **list;
} data_array;

struct keyval {
  pdfout_data *key;
  pdfout_data *value;
};

typedef struct data_hash_s {
  pdfout_data super;
  int len, cap;
  struct keyval *list;
} data_hash;

bool
pdfout_data_is_scalar (fz_context *ctx, pdfout_data *data)
{
  return (data->type == SCALAR);
}

bool
pdfout_data_is_array (fz_context *ctx, pdfout_data *data)
{
  return (data->type == ARRAY);
}

bool
pdfout_data_is_hash (fz_context *ctx, pdfout_data *data)
{
  return (data->type == HASH);
}

static data_hash *to_hash (pdfout_data *data)
{
  assert (data->type == HASH);
  return (data_hash *) data;
}

static data_array *to_array (pdfout_data *data)
{
  assert (data->type == ARRAY);
  return (data_array *) data;
}

static data_scalar *to_scalar (pdfout_data *data)
{
  assert (data->type == SCALAR);
  return (data_scalar *) data;
}


pdfout_data *
pdfout_data_scalar_new (fz_context *ctx, const char *value, int len)
{
  data_scalar *result = fz_malloc_struct (ctx, data_scalar);
  
  result->super.type = SCALAR;
  
  result->len = len;

  result->value = fz_malloc (ctx, len + 1);
  memcpy (result->value, value, len);
  result->value[len] = 0;
  
  return (pdfout_data *) result;
}

pdfout_data *
pdfout_data_array_new (fz_context *ctx)
{
  data_array *result = fz_malloc_struct (ctx, data_array);

  result->super.type = ARRAY;

  result->len = 0;
  result->cap = 0;
  
  fz_try (ctx)
  {
    result->list = NULL;
    result->list = pdfout_x2nrealloc (ctx, result->list, &result->cap,
				      pdfout_data *);
  }
  fz_catch (ctx)
  {
    free (result);
    fz_rethrow (ctx);
  }
  
  return (pdfout_data *) result;
}


pdfout_data *
pdfout_data_hash_new (fz_context *ctx)
{
  data_hash *result = fz_malloc_struct (ctx, data_hash);

  result->super.type = HASH;

  result->len = 0;
  result->cap = 0;
  
  fz_try (ctx)
  {
    result->list = NULL;
    result->list =
      pdfout_x2nrealloc (ctx, result->list, &result->cap, struct keyval);
  }
  fz_catch (ctx)
  {
    free (result);
    fz_rethrow (ctx);
  }
  
  return (pdfout_data *) result;

}

static void drop_scalar (fz_context *ctx, data_scalar *s)
{
  free (s->value);
  free (s);
}

static void drop_array (fz_context *ctx, data_array *a)
{
  for (int i = 0; i < a->len; ++i)
    pdfout_data_drop (ctx, a->list[i]);
  free (a->list);
  free (a);
}

static void drop_hash (fz_context *ctx, data_hash *h)
{
  for (int i = 0; i < h->len; ++i)
    {
      pdfout_data_drop (ctx, h->list[i].key);
      pdfout_data_drop (ctx, h->list[i].value);
    }
  free (h->list);
  free (h);
}

void
pdfout_data_drop (fz_context *ctx, pdfout_data *data)
{
  switch (data->type)
    {
    case SCALAR: drop_scalar (ctx, to_scalar (data)); break;
    case ARRAY: drop_array (ctx, to_array (data)); break;
    case HASH:  drop_hash (ctx, to_hash (data)); break;
    default:
      abort ();
    }
}


char *
pdfout_data_scalar_get (fz_context *ctx, pdfout_data *scalar, int *len)
{
  data_scalar *s = to_scalar (scalar);
  *len = s->len;
  return s->value;
}

char *
pdfout_data_scalar_get_string (fz_context *ctx, pdfout_data *scalar)
{
  int len;
  char *string = pdfout_data_scalar_get (ctx, scalar, &len);

  for (int i = 0; i < len; ++i)
    if (string[i] == 0)
      pdfout_throw (ctx, "string contains embedded null byte");

  return string;
}

int
pdfout_data_array_len (fz_context *ctx, pdfout_data *array)
{
  data_array *a = to_array (array);
  return a->len;
}

void
pdfout_data_array_push (fz_context *ctx, pdfout_data *array, pdfout_data *entry)
{
  data_array *a = to_array (array);

  if (a->cap == a->len)
    a->list = pdfout_x2nrealloc (ctx, a->list, &a->cap, pdfout_data *);
  
  a->list[a->len++] = entry;
}
  
pdfout_data *
pdfout_data_array_get (fz_context *ctx, pdfout_data *array, int pos)
{
  data_array *a = to_array (array);
  assert (pos < a->len);
  return a->list[pos];
}

int
pdfout_data_hash_len (fz_context *ctx, pdfout_data *hash)
{
  data_hash *h = to_hash (hash);

  return h->len;
}

void
pdfout_data_hash_push (fz_context *ctx, pdfout_data *hash,
		       pdfout_data *key, pdfout_data *value)
{
  data_hash *h = to_hash (hash);
  data_scalar *k = to_scalar (key);
  
  /* Is the key already there?  */
  for (int i = 0; i < h->len; ++i)
    {
      data_scalar *k_i = to_scalar (h->list[i].key);
      if (k_i->len == k->len && memcmp (k_i->value, k->value, k->len) == 0)
	pdfout_throw (ctx, "key '%.*s' is already present in hash",
		      k->len, k->value);
    }
		  
  if (h->cap == h->len)
    h->list = pdfout_x2nrealloc (ctx, h->list, &h->cap, struct keyval);

  h->list[h->len].key = key;
  h->list[h->len].value = value;
  ++h->len;
}

pdfout_data *
pdfout_data_hash_get_key (fz_context *ctx, pdfout_data *hash, int pos)
{
  data_hash *h = to_hash (hash);
  assert (pos < h->len);

  return h->list[pos].key;
}

pdfout_data *
pdfout_data_hash_get_value (fz_context *ctx, pdfout_data *hash, int pos)
{
  data_hash *h = to_hash (hash);
  assert (pos < h->len);

  return h->list[pos].value;
}

void
pdfout_data_hash_get_key_value (fz_context *ctx, pdfout_data *hash,
				char **key, char **value, int *value_len,
				int i)
{
  pdfout_data *k = pdfout_data_hash_get_key (ctx, hash, i);
  *key = pdfout_data_scalar_get_string (ctx, k);
  
  pdfout_data *v = pdfout_data_hash_get_value (ctx, hash, i);
  if (pdfout_data_is_scalar (ctx , v) == false)
    pdfout_throw (ctx, "value not a scalar in pdfout_data_hash_get_key_value");

  *value = pdfout_data_scalar_get (ctx, v, value_len);
}

void
pdfout_data_hash_push_key_value (fz_context *ctx, pdfout_data *hash,
				 const char *key, const char *value,
				 int value_len)
{
  pdfout_data *k = pdfout_data_scalar_new (ctx, key, strlen (key));
  pdfout_data *v = pdfout_data_scalar_new (ctx, value, value_len);
  pdfout_data_hash_push (ctx, hash, k, v);
}

pdfout_data *
pdfout_data_hash_gets (fz_context *ctx, pdfout_data *hash, char *key)
{
  data_hash *h = to_hash (hash);
  
  size_t len = strlen (key);

  for (int i = 0; i < len; ++i)
    {
      data_scalar *k = to_scalar (h->list[i].key);
      if (len == k->len && memcmp (key, k->value, len) == 0)
	return (pdfout_data *) k;
    }
  return NULL;
}

/* Comparison  */

static int cmp_array (fz_context *ctx, pdfout_data *x, pdfout_data *y)
{
  data_array *a = to_array (x);
  data_array *b = to_array (y);

  if (a->len != b->len)
    return 1;
  
  for (int i = 0; i < a->len; ++i)
    {
      if (pdfout_data_cmp (ctx, a->list[i], b->list[i]))
	return 1;
    }
  return 0;
}

static int cmp_hash (fz_context *ctx, pdfout_data *x, pdfout_data *y)
{
  data_hash *a = to_hash (x);
  data_hash *b = to_hash (y);

  if (a->len != b->len)
    return 1;
  
  for (int i = 0; i < a->len; ++i)
    {
      if (pdfout_data_cmp (ctx, a->list[i].key, b->list[i].key)
	  || pdfout_data_cmp (ctx, a->list[i].value, b->list[i].value))
	return 1;
    }
  return 0;
}

int
pdfout_data_cmp (fz_context *ctx, pdfout_data *x, pdfout_data *y)
{
  if (x->type != y->type)
    return 1;
  if (x->type == ARRAY)
    return cmp_array (ctx, x, y);
  else if (x->type == HASH)
    return cmp_hash (ctx, x, y);

  data_scalar *a = to_scalar (x);
  data_scalar *b = to_scalar (y); 

  if (a->len == b->len && memcmp (a->value, b->value, a->len) == 0)
    return 0;
  else
    return 1;
}

/* Parser and emitter stuff.  */


void
pdfout_parser_drop (fz_context *ctx, pdfout_parser *parser)
{
  parser->drop (ctx, parser);
}

pdfout_data *
pdfout_parser_parse (fz_context *ctx, pdfout_parser *parser)
{
  return parser->parse (ctx, parser);
}

void
pdfout_emitter_drop (fz_context *ctx, pdfout_emitter *emitter)
{
  emitter->drop (ctx, emitter);
}

void
pdfout_emitter_emit (fz_context *ctx, pdfout_emitter *emitter,
		     pdfout_data *data)
{
  return emitter->emit (ctx, emitter, data);
}

