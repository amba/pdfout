#include "common.h"

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

static const char*
type_to_string (enum data_type type)
{
  switch (type)
    {
    case SCALAR:
      return "Scalar";
    case ARRAY:
      return "ARRAY";
    case HASH:
      return "HASH";
    default:
      abort();
    }
}

static void
assert_type (fz_context *ctx, pdfout_data *data, enum data_type expected)
{
  enum data_type type = data->type;
  
  if (type != expected)
    pdfout_throw (ctx, "Expected pdfout_data type %s, got %s",
		  type_to_string(expected), type_to_string(type));
}

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

static data_hash *to_hash (fz_context *ctx, pdfout_data *data)
{
  assert_type (ctx, data, HASH);
  return (data_hash *) data;
}

static data_array *to_array (fz_context *ctx, pdfout_data *data)
{
  assert_type (ctx, data, ARRAY);
  return (data_array *) data;
}

static data_scalar *to_scalar (fz_context *ctx, pdfout_data *data)
{
  assert_type (ctx, data, SCALAR);
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
    case SCALAR: drop_scalar (ctx, to_scalar (ctx, data)); break;
    case ARRAY: drop_array (ctx, to_array (ctx, data)); break;
    case HASH:  drop_hash (ctx, to_hash (ctx, data)); break;
    default:
      abort ();
    }
}


char *
pdfout_data_scalar_get (fz_context *ctx, pdfout_data *scalar, int *len)
{
  data_scalar *s = to_scalar (ctx, scalar);
  *len = s->len;
  return s->value;
}

bool
pdfout_data_scalar_eq (fz_context *ctx, pdfout_data *scalar, const char *s)
{
  int len;
  const char *value = pdfout_data_scalar_get (ctx, scalar, &len);
  if (len == strlen (s) && strcmp (s, value) == 0)
    return true;
  else
    return false;
}

int
pdfout_data_array_len (fz_context *ctx, pdfout_data *array)
{
  data_array *a = to_array (ctx, array);
  return a->len;
}

void
pdfout_data_array_push (fz_context *ctx, pdfout_data *array, pdfout_data *entry)
{
  data_array *a = to_array (ctx, array);

  if (a->cap == a->len)
    a->list = pdfout_x2nrealloc (ctx, a->list, &a->cap, pdfout_data *);
  
  a->list[a->len++] = entry;
}
  
pdfout_data *
pdfout_data_array_get (fz_context *ctx, pdfout_data *array, int pos)
{
  data_array *a = to_array (ctx, array);
  assert (pos < a->len);
  return a->list[pos];
}

int
pdfout_data_hash_len (fz_context *ctx, pdfout_data *hash)
{
  data_hash *h = to_hash (ctx, hash);

  return h->len;
}

void
pdfout_data_hash_push (fz_context *ctx, pdfout_data *hash,
		       pdfout_data *key, pdfout_data *value)
{
  data_hash *h = to_hash (ctx, hash);
  data_scalar *k = to_scalar (ctx, key);
  
  /* Is the key already there?  */
  for (int i = 0; i < h->len; ++i)
    {
      data_scalar *k_i = to_scalar (ctx, h->list[i].key);
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
  data_hash *h = to_hash (ctx, hash);
  assert (pos < h->len);

  return h->list[pos].key;
}

pdfout_data *
pdfout_data_hash_get_value (fz_context *ctx, pdfout_data *hash, int pos)
{
  data_hash *h = to_hash (ctx, hash);
  assert (pos < h->len);

  return h->list[pos].value;
}

static char *
scalar_get_string (fz_context *ctx, pdfout_data *scalar)
{
  int len;
  char *string = pdfout_data_scalar_get (ctx, scalar, &len);

  for (int i = 0; i < len; ++i)
    if (string[i] == 0)
      pdfout_throw (ctx, "string contains embedded null byte");

  return string;
}

/* Scalar convenience functions.  */

pdf_obj *
pdfout_data_scalar_to_pdf_name (fz_context *ctx, pdf_document *doc,
				pdfout_data *scalar)
{
  const char *s = scalar_get_string (ctx, scalar);
  return pdf_new_name (ctx, doc, s);
}
  
pdf_obj *
pdfout_data_scalar_to_pdf_str (fz_context *ctx, pdf_document *doc,
			       pdfout_data *scalar)
{
  int len;
  char *s = pdfout_data_scalar_get (ctx, scalar, &len);
  return pdfout_utf8_to_str_obj (ctx, doc, s, len);
}

pdf_obj *
pdfout_data_scalar_to_pdf_int (fz_context *ctx, pdf_document *doc,
			       pdfout_data *scalar)
{
  char *s = scalar_get_string (ctx, scalar);
  int n = pdfout_strtoint_null (ctx, s);
  return pdf_new_int (ctx, doc, n);
}

pdf_obj *
pdfout_data_scalar_to_pdf_real (fz_context *ctx, pdf_document *doc,
				pdfout_data *scalar)
{
  char *s = scalar_get_string (ctx, scalar);
  float f = pdfout_strtof (ctx, s);
  return pdf_new_real (ctx, doc, f);
}

pdfout_data *
pdfout_data_scalar_from_pdf (fz_context *ctx, pdf_obj *obj)
{
  const char *s;
  if (pdf_is_null (ctx, obj))
    return pdfout_data_scalar_new (ctx, "null", strlen ("null"));
  else if (pdf_is_bool (ctx, obj))
    {
      if (pdf_to_bool (ctx, obj))
	s = "true";
      else
	s = "false";
      return pdfout_data_scalar_new (ctx, s, strlen (s));
    }
  else if (pdf_is_name (ctx, obj))
    {
      s = pdf_to_name (ctx, obj);
      return pdfout_data_scalar_new (ctx, s, strlen (s));
    }
  else if (pdf_is_string (ctx, obj))
    {
      int len;
      char *str = pdfout_str_obj_to_utf8 (ctx, obj, &len);
      pdfout_data *result =  pdfout_data_scalar_new (ctx, str, len);
      free (str);
      return result;
    }
  else if (pdf_is_int (ctx, obj))
    {
      int n = pdf_to_int (ctx, obj);
      char buf[200];
      int len = pdfout_snprintf (ctx, buf, "%d", n);
      return pdfout_data_scalar_new (ctx, buf, len);
    }
  else if (pdf_is_real (ctx, obj))
    {
      float f = pdf_to_real (ctx, obj);
      char buf[200];
      int len = pdfout_snprintf (ctx, buf, "%g", f);
      return pdfout_data_scalar_new (ctx, buf, len);
    }
  else
    abort();
  
}
/* Hash convenience functions */


void
pdfout_data_hash_get_key_value (fz_context *ctx, pdfout_data *hash,
				char **key, char **value, int *value_len,
				int i)
{
  pdfout_data *k = pdfout_data_hash_get_key (ctx, hash, i);
  *key = scalar_get_string (ctx, k);
  
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
pdfout_data_hash_gets (fz_context *ctx, pdfout_data *hash, const char *key)
{
  int hash_len = pdfout_data_hash_len (ctx, hash);
    
  size_t key_len = strlen (key);

  for (int i = 0; i < hash_len; ++i)
    {
      pdfout_data *key_data = pdfout_data_hash_get_key (ctx, hash, i);
      int len;
      char *value = pdfout_data_scalar_get (ctx, key_data, &len);
      if (len == key_len && memcmp (key, value, len) == 0)
	return pdfout_data_hash_get_value (ctx, hash, i);
    }
  return NULL;
}

/* Comparison  */

static int cmp_array (fz_context *ctx, pdfout_data *x, pdfout_data *y)
{
  data_array *a = to_array (ctx, x);
  data_array *b = to_array (ctx, y);

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
  data_hash *a = to_hash (ctx, x);
  data_hash *b = to_hash (ctx, y);

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

  data_scalar *a = to_scalar (ctx, x);
  data_scalar *b = to_scalar (ctx, y); 

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

