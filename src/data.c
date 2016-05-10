#include "common.h"
#include "data.h"
#Include "c-ctype.h"
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
  pdfout_data SUPER;
  int len;
  char *value;
} data_scalar;

typedef struct data_array_s {
  pdfout_data SUPER;
  int len, cap;
  pdfout_data **list;
} data_array;

struct keyval {
  pdfout_data *key;
  pdfout_data *value;
};

typedef struct data_hash_s {
  pdfout_data SUPER;
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

pdfout_data *
pdfout_data_create_scalar (fz_context *ctx, const char *value, int len)
{
  data_scalar *result = fz_malloc_struct (ctx, data_scalar);
  
  result->SUPER.type = SCALAR;
  
  result->len = len;

  result->value = fz_malloc (ctx, len);
  memcpy (result->value, value, len);
  
  return (pdfout_data *) result;
}

pdfout_data *
pdfout_data_create_array (fz_context *ctx)
{
  data_array *result = fz_malloc_struct (ctx, data_array);

  result->SUPER.type = ARRAY;

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
pdfout_data_create_hash (fz_context *ctx)
{
  data_hash *result = fz_malloc_struct (ctx, data_hash);

  result->SUPER.type = HASH;

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
    case SCALAR: drop_scalar (ctx, (data_scalar *) data); break;
    case ARRAY: drop_array (ctx, (data_array *) data); break;
    case HASH:  drop_hash (ctx, (data_hash *) data); break;
    default:
      abort ();
    }
}


char *
pdfout_data_scalar_get (fz_context *ctx, pdfout_data *scalar, int *len)
{
  assert (scalar->type == SCALAR);
  data_scalar *s = (data_scalar *) scalar;
  *len = s->len;
  return s->value;
}

int
pdfout_data_array_len (fz_context *ctx, pdfout_data *array)
{
  assert (array->type == ARRAY);
  data_array *a = (data_array *) array;
  return a->len;
}

void
pdfout_data_array_push (fz_context *ctx, pdfout_data *array, pdfout_data *entry)
{
  assert (array->type == ARRAY);
  data_array *a = (data_array *) array;

  if (a->cap == a->len)
    a->list = pdfout_x2nrealloc (ctx, a->list, &a->cap, pdfout_data *);
  
  a->list[a->len++] = entry;
}
  
pdfout_data *
pdfout_data_array_get (fz_context *ctx, pdfout_data *array, int pos)
{
  assert (array->type == ARRAY);
  data_array *a = (data_array *) array;
  assert (pos < a->len);
  return a->list[pos];
}

int
pdfout_data_hash_len (fz_context *ctx, pdfout_data *hash)
{
  assert (hash->type == HASH);
  data_hash *h = (data_hash *) hash;

  return h->len;
}

void
pdfout_data_hash_push (fz_context *ctx, pdfout_data *hash,
		       pdfout_data *key, pdfout_data *value)
{
  assert (hash->type == HASH);
  data_hash *h = (data_hash *) hash;

  if (h->cap == h->len)
    h->list = pdfout_x2nrealloc (ctx, h->list, &h->cap, struct keyval);

  h->list[h->len].key = key;
  h->list[h->len].value = value;
  ++h->len;
}

pdfout_data *
pdfout_data_hash_get_key (fz_context *ctx, pdfout_data *hash, int pos)
{
  assert (hash->type == HASH);
  data_hash *h = (data_hash *) hash;
  assert (pos < h->len);

  return h->list[pos].key;
}

pdfout_data *
pdfout_data_hash_get_value (fz_context *ctx, pdfout_data *hash, int pos)
{
  assert (hash->type == HASH);
  data_hash *h = (data_hash *) hash;
  assert (pos < h->len);

  return h->list[pos].value;
}
  
pdfout_data *
pdfout_data_hash_gets (fz_context *ctx, pdfout_data *hash, char *key)
{
  assert (hash->type == HASH);
  data_hash *h = (data_hash *) hash;
  
  size_t len = strlen (key);

  for (int i = 0; i < len; ++i)
    {
      data_scalar *k = (data_scalar *) h->list[i].key;
      if (len == k->len && memcmp (key, k->value, len) == 0)
	return (pdfout_data *) k;
    }
  return NULL;
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

