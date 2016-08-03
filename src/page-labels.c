#include "common.h"
#include "page-labels.h"
#include "charset-conversion.h"
#include <argmatch.h>

/* Update PDF's page labels.  */


static bool
has_null_bytes (const char *s, int len)
{
  for (int i = 0; i < len; ++i)
    if (s[i] == 0)
      return true;
  
  return false;
}

const char *
get_hash_string (fz_context *ctx, pdfout_data *hash, const char *key)
{
  pdfout_data *key_data = pdfout_data_hash_gets (ctx, hash, key);

  if (key_data == NULL)
    return NULL;
  
  if (pdfout_data_is_scalar (ctx, key_data) == false)
    pdfout_throw (ctx, "expected scalar for key '%s'", key);

  int len;
  const char *s = pdfout_data_scalar_get (ctx, key_data, &len);

  if (has_null_bytes(s, len))
    pdfout_throw (ctx, "value of key '%s' has embedded null bytes", key);

  return s;
}

static void
check_page_labels (fz_context *ctx, pdfout_data *labels)
{
  int len = pdfout_data_array_len (ctx, labels);
  int previous_page;
  for (int i = 0; i < len; ++i)
    {
      pdfout_data *hash = pdfout_data_array_get (ctx, labels, i);
      if (pdfout_data_is_hash (ctx, hash) == false)
	pdfout_throw (ctx, "expected hash element in page label");

      const char *page_string = get_hash_string (ctx, hash, "page");
      if (page_string == NULL)
	pdfout_throw (ctx, "missing mandatory key 'page'");
      
      unsigned page = pdfout_strtoui(ctx, page_string);
      if (page < 1)
	pdfout_throw (ctx, "page must be >= 1");
      
      if (i == 0 && page > 1)
	pdfout_throw (ctx, "first page must be 1");

      if (page <= previous_page)
	pdfout_throw (ctx, "page numbers must increase");

      const char* first_str = get_hash_string (ctx, hash, "first");

      if (first_str)
	{
	  if (pdfout_strtoui (ctx, first_str) < 1)
	    pdfout_throw (ctx, "value of key 'first' must be >= 1");
	}
      
      get_hash_string (ctx, hash, "prefix");
      
      const char *style = get_hash_string (ctx, hash, "style");
      if (style
	  && strcmp (style, "arabic") && strcmp (style, "Roman")
	  && strcmp (style, "roman") && strcmp (style, "Letters")
	  && strcmp (style, "letters"))
	pdfout_throw (ctx, "invalid style '%s'", style);
	
    }
  
}

#define MSG(fmt, args...)					\
  pdfout_msg ("updating PDF: " fmt, ## args)
/* FIXME: const char **parameter for all functions that produce messages?  */
int pdfout_page_labels_set (fz_context *ctx, pdf_document *doc,
			    const pdfout_page_labels_t *labels)
{
  int num, i;
  pdf_obj *root, *labels_obj, *array_obj;

  if (labels)
    {
      num = pdfout_page_labels_length (labels);
      assert (num >= 1);
      /* FIXME */
      /* assert (labels->array[0].page == 0); */
    }
  
  root = pdf_dict_get (ctx, pdf_trailer (ctx, doc), PDF_NAME_Root);
  
  if (root == NULL)
    {
      MSG ("no document catalog, cannot set/unset page labels");
      return 1;
    }
  
  if (labels == NULL)
    {
      MSG ("removing page labels");
      pdf_dict_dels (ctx, root, "PageLabels");
      return 0;
    }

  array_obj = pdf_new_array (ctx, doc, 2 * num);
  
  for (i = 0; i < num; ++i)
    {
      pdf_obj *dict_obj, *string_obj;
      pdfout_page_labels_style_t style;
      char *text, *prefix;
      int text_len;
      pdfout_page_labels_mapping_t *mapping;

      mapping = pdfout_page_labels_get_mapping (labels, i);
      dict_obj = pdf_new_dict (ctx, doc, 3);

      prefix = mapping->prefix;
      if (prefix)
	{
	  text = pdfout_utf8_to_pdf (ctx, prefix, strlen (prefix), &text_len);
	  /* FIXME: check INT_MAX overflow.  */
	  string_obj = pdf_new_string (ctx, doc, text, text_len);
	  pdf_dict_puts_drop (ctx, dict_obj, "P", string_obj);
	  free (text);
	}
	 
      if (mapping->start)
	pdf_dict_puts_drop (ctx, dict_obj, "St",
			    pdf_new_int (ctx, doc, mapping->start));
      style = mapping->style;
      if (style == PDFOUT_PAGE_LABELS_ARABIC)
	pdf_dict_puts_drop (ctx, dict_obj, "S", pdf_new_name (ctx, doc, "D"));
      else if (style == PDFOUT_PAGE_LABELS_UPPER_ROMAN)
	pdf_dict_puts_drop (ctx, dict_obj, "S", pdf_new_name (ctx, doc, "R"));
      else if (style == PDFOUT_PAGE_LABELS_LOWER_ROMAN)
	pdf_dict_puts_drop (ctx, dict_obj, "S", pdf_new_name (ctx, doc, "r"));
      else if (style == PDFOUT_PAGE_LABELS_UPPER)
	pdf_dict_puts_drop (ctx, dict_obj, "S", pdf_new_name (ctx, doc, "A"));
      else if (style == PDFOUT_PAGE_LABELS_LOWER)
	pdf_dict_puts_drop (ctx, dict_obj, "S", pdf_new_name (ctx, doc, "a"));
      else if (style)
	abort ();
      
      pdf_array_push_drop (ctx, array_obj,
			   pdf_new_int (ctx, doc, mapping->page));
      pdf_array_push_drop (ctx, array_obj, dict_obj);
    }
  
  labels_obj = pdf_new_dict (ctx, doc, 1);
  pdf_dict_puts_drop (ctx, labels_obj, "Nums", array_obj);
  pdf_dict_puts_drop (ctx, root, "PageLabels", labels_obj);
  
  return 0;
}

/* Parse YAML to pdfout_page_labels_t.  */

#undef MSG
#define MSG(fmt, args...) \
  pdfout_msg ("pdfout_page_labels_from_yaml: " fmt, ## args)

#define MSG_RETURN(status, fmt, args...)	\
  do						\
    {						\
      MSG (fmt, ## args);			\
      return status;				\
    }						\
  while (0)

#define MSG_GOTO(label, fmt, args...)		\
  do						\
    {						\
      MSG (fmt, ## args);			\
      goto label;				\
    }						\
  while (0)
    

#define PARSE_OR_RETURN(status, parser, event)		\
  do							\
    if (pdfout_yaml_parser_parse (parser, event))	\
      return status;					\
  while (0)

#define PARSE_OR_GOTO(label, parser, event)		\
  do							\
    if (pdfout_yaml_parser_parse (parser, event))	\
      goto label;					\
  while (0)


/* Return values:
   0: *KEY and *VALUE are allocated.
   1: Encountered a mapping end event.
   2: Error.  */
static int
get_scalar_mapping_pair (char **key, char **value,
			 yaml_parser_t *parser, yaml_event_t *event)
{
  char *key_value[2];
  int i;
  
  for (i = 0; i < 2; ++i)
    key_value[i] = NULL;
  
  for (i = 0; i < 2; ++i)
    {
      PARSE_OR_GOTO (error, parser, event);
      
      if (pdfout_yaml_is_mapping_end (event))
	/* Mapping holds even number of nodes, so no need to deallocate.  */
	return 1;
      else if (pdfout_yaml_is_scalar (event) == false)
	MSG_GOTO (error, "expected scalar");
      
      key_value[i] = xstrdup (pdfout_yaml_scalar_value (event));
    }

  *key = key_value[0];
  *value = key_value[1];
  
  return 0;
      
 error:
  for (i = 0; i < 2; ++i)
    free (key_value[i]);
  return 2;
}

#define STREQ(s1, s2) (strcasecmp (s1, s2) == 0)

static int
process_scalar_mapping_pair (char *key, char *value,
					pdfout_page_labels_mapping_t *mapping)
{
  if (STREQ (key, "page"))
    mapping->page = pdfout_strtoui (value);
  else if (STREQ (key, "style"))
    {
      const char * styles[] = {"arabic", "Roman", "roman",
			       "Letters", "letters", NULL};
      int style = argmatch (value, styles, NULL, 0);
      if (style < 0)
	MSG_RETURN (1, "unknown numbering style '%s'", value);
      mapping->style = style + 1;
    }
  else if (STREQ (key, "prefix"))
    mapping->prefix = xstrdup (value);
  else if (STREQ (key, "start"))
    mapping->start = pdfout_strtoui (value);
  return 0;
}

static int
get_page_labels_mapping (pdfout_page_labels_t *labels, yaml_parser_t *parser,
			 yaml_event_t *event)
{
  char *key , *value;
  int rv, rv_key_val, result;
  pdfout_page_labels_mapping_t mapping = {0};
    
  while (1)
    {
      rv = get_scalar_mapping_pair (&key, &value, parser, event);
      switch (rv)
	{
	case 0:
	  rv_key_val = process_scalar_mapping_pair (key, value, &mapping);
	  free (key);
	  free (value);
	  if (rv_key_val)
	    {
	      result = 1;
	      goto end;
	    }
	  break;
	case 1:
	  /* Reached end of mapping.  */
	  if (mapping.page-- == 0)
	    {
	      result = 1;
	      MSG ("missing key 'page'");
	    }
	  else
	    result = pdfout_page_labels_push (labels, &mapping);
	  goto end;
	case 2:
	  return 1;
	  break;
	default:
	  abort ();
	}
    }
  
 end:
  free (mapping.prefix);
  return result;
}


/* Return 0 for success, 1 for empty sequence and 2 on error.  */
static int
get_page_labels_sequence (pdfout_page_labels_t *labels, yaml_parser_t *parser,
			  yaml_event_t *event)
{
  bool have_mapping = false;
  while (1)
    {
      PARSE_OR_RETURN (2, parser, event);
      if (pdfout_yaml_is_mapping_start (event))
	{
	  if (get_page_labels_mapping (labels, parser, event))
	    return 2;
	  else
	    have_mapping = true;
	}
      else if (pdfout_yaml_is_sequence_end (event))
	break;
      else
	MSG_RETURN (2, "unexpected event");
    }

  if (have_mapping == false)
    return 1;
  
  return 0;
}

static int
page_labels_from_yaml (pdfout_page_labels_t **labels_ptr,
		       yaml_parser_t *parser, yaml_event_t *event)
{
  pdfout_page_labels_t *labels;
  int rv;
  
  rv = pdfout_yaml_parser_start (parser, event);
  if (rv == 2)
    return 1;
  else if (rv == 1)
    {
      *labels_ptr = NULL;
      MSG_RETURN (0, "empty YAML document");
    }
  
  PARSE_OR_RETURN (1, parser, event);

  if (pdfout_yaml_is_sequence_start (event) == false)
    MSG_RETURN (1, "expected sequence start event");
  
  labels = *labels_ptr = pdfout_page_labels_new ();
  
  rv = get_page_labels_sequence (labels, parser, event);
  if (rv)
    pdfout_page_labels_free (labels);
  if (rv == 1)
    /* Empty sequence.  */
    *labels_ptr = NULL;
  else if (rv == 2)
    return 1;

  /* Parse document-end event.  */
  PARSE_OR_RETURN (1, parser, event);
  assert (pdfout_yaml_is_document_end (event));
  
  return 0;
}

int
pdfout_page_labels_from_yaml (pdfout_page_labels_t **labels_ptr,
			      yaml_parser_t *parser)
{
  int rv;
  yaml_event_t *event = pdfout_yaml_event_new ();
  
  rv = page_labels_from_yaml (labels_ptr, parser, event);

  pdfout_yaml_event_free (event);
  
  return rv;
}

static int
emit_key_val (yaml_emitter_t *emitter, yaml_event_t *event, const char *key,
	      const char *value)
{
  return (pdfout_yaml_scalar_event (emitter, event, key)
	  || pdfout_yaml_scalar_event (emitter, event, value));
}

static int
mapping_to_yaml (yaml_emitter_t *emitter, yaml_event_t *event,
		 const pdfout_page_labels_mapping_t *mapping)
{
  char buffer[256];
  
  if (pdfout_yaml_mapping_start_event (emitter, event,
				       YAML_BLOCK_MAPPING_STYLE))
    return 1;

  /* page */
  assert (mapping->page >= 0);
  PDFOUT_SNPRINTF_OLD (buffer, "%d", mapping->page + 1);

  if (emit_key_val (emitter, event, "page", buffer))
    return 1;
  
  /* style */
  if (mapping->style)
    {
      const char *styles[] = {"arabic", "Roman", "roman", "Letters",
			      "letters"};
      
      assert (1 <= mapping->style && mapping->style <= 5);
      if (emit_key_val (emitter, event, "style", styles[mapping->style - 1]))
	return 1;
    }
  if (mapping->start)
    {
      assert (mapping->start > 0);
      PDFOUT_SNPRINTF_OLD (buffer, "%d", mapping->start);
      if (emit_key_val (emitter, event, "start", buffer))
	return 1;
    }
  
  if (mapping->prefix)
    if (emit_key_val (emitter, event, "prefix", mapping->prefix))
      return 1;

  return pdfout_yaml_mapping_end_event (emitter, event);
}

static int
page_labels_to_yaml (yaml_emitter_t *emitter, yaml_event_t *event,
		     const pdfout_page_labels_t *labels)
{
  int len;
  int i;
  const pdfout_page_labels_mapping_t *mapping;

  len = pdfout_page_labels_length (labels);
  assert (len > 0);
  if (pdfout_yaml_emitter_open (emitter, event)
      || pdfout_yaml_document_start_event (emitter, event, NULL, true)
      || pdfout_yaml_sequence_start_event (emitter, event,
					   YAML_BLOCK_SEQUENCE_STYLE))
    return 1;
  
  for (i = 0; i < len; ++i)
    {
      mapping = pdfout_page_labels_get_mapping (labels, i);
      if (mapping_to_yaml (emitter, event, mapping))
	return 1;
    }
  if (pdfout_yaml_sequence_end_event (emitter, event)
      || pdfout_yaml_document_end_event (emitter, event, true))
    return 1;
  
  return 0;
}

/* Convert page labels to YAML.  */

int
pdfout_page_labels_to_yaml (yaml_emitter_t *emitter,
			    const pdfout_page_labels_t *labels)
{
  
  yaml_event_t *event = pdfout_yaml_event_new ();
  
  assert (labels);

  int rv = page_labels_to_yaml (emitter, event, labels);

  pdfout_yaml_event_free (event);
  return rv;
}

/* Extract page labels.  */
#undef MSG
#define MSG(fmt, args...) pdfout_msg ("pdfout_page_labels_get: " fmt, ## args)
#undef STREQ
#define STREQ(s1, s2) (strcmp (s1, s2) == 0)
static int
page_labels_get (pdfout_page_labels_t *labels, fz_context *ctx,
		 pdf_document *doc)
{
  bool broken = false;
  pdf_obj *array, *labels_obj;
  int length, i;
  
  labels_obj = pdf_dict_getp (ctx, pdf_trailer (ctx, doc), "Root/PageLabels");
  
  if (labels_obj == NULL)
    MSG_RETURN (0, "no 'PageLabels' entry in document catalogue");
  
  array = pdf_dict_gets (ctx, labels_obj, "Nums");
  if (pdf_is_array (ctx, array) == false)
    MSG_RETURN (1, "key 'Nums' not an array");
  
  length = pdf_array_len (ctx, array);
  if (length == 0)
    MSG_RETURN (1, "empty 'PageLabels' entry");
  
  if (length % 2)
    {
      MSG ("uneven number in Nums array");
      broken = true;
    }
  
  for (i = 0; i < length / 2; ++i)
    {
      pdfout_page_labels_mapping_t mapping = {0};
      pdf_obj *dict, *object;

      object = pdf_array_get (ctx, array, 2 * i);
      
      if (pdf_is_int (ctx, object) == false)
	{
	  MSG ("key in number tree not an int");
	  broken = true;
	  continue;
	}
      
      mapping.page = pdf_to_int (ctx, object);
      
      if (mapping.page < 0)
	{
	  MSG ("key in number tree is < 0");
	  broken = true;
	  continue;
	}
      
      dict = pdf_array_get (ctx, array, 2 * i + 1);
      if (pdf_is_dict (ctx, dict) == false)
	{
	  MSG ("value in number tree not a dict");
	  broken = true;
	  continue;
	}
      
      object = pdf_dict_get (ctx, dict, PDF_NAME_S);
      if (object)
	{
	  if (pdf_is_name (ctx, object) == false)
	    {
	      MSG ("'S' not a name object");
	      broken = true;
	    }
	  else
	    {
	      /* pdf_to_name returns the empty string, not NULL, on all errors,
		 so the strcmps are allowed.  */
	      if (pdf_name_eq (ctx, object, PDF_NAME_D))
		mapping.style = PDFOUT_PAGE_LABELS_ARABIC;
	      else if (pdf_name_eq (ctx, object, PDF_NAME_R))
		mapping.style = PDFOUT_PAGE_LABELS_UPPER_ROMAN;
	      else if (pdf_name_eq (ctx, object, PDF_NAME_A))
		mapping.style = PDFOUT_PAGE_LABELS_UPPER;
	      else
		{
		  /* FIXME once PDF_NAMES for "r" and "a" available.  */
		  const char *style = pdf_to_name (ctx, object);
		  if (STREQ (style, "r"))
		    mapping.style = PDFOUT_PAGE_LABELS_LOWER_ROMAN;
		  else if (STREQ (style, "a"))
		    mapping.style = PDFOUT_PAGE_LABELS_LOWER;
		  else
		    {
		      MSG ("unknown numbering style '%s'", style);
		      broken = true;
		    }
		}
	    }
	}
      object = pdf_dict_gets (ctx, dict, "St");
      if (object)
	{
	  if (pdf_is_int (ctx, object) == false)
	    {
	      MSG ("'St' not an int");
	      broken = true;
	    }
	  mapping.start = pdf_to_int (ctx, object);
	  if (mapping.start < 1)
	    {
	      MSG ("value %d of 'St' is < 1", mapping.start);
	      broken = true;
	      mapping.start = 0;
	    }
	}
      
      object = pdf_dict_get (ctx, dict, PDF_NAME_P);
      if (object)
	{
	  if (pdf_is_string (ctx, object) == false)
	    {
	      MSG ("'P' not a string");
	      broken = true;
	    }
	  else
	    {
	      const char *pdf_buf = pdf_to_str_buf (ctx, object);
	      int pdf_buf_len = pdf_to_str_len (ctx, object);
	      if (pdf_buf_len)
		{
		  int utf8_buf_len;
		  mapping.prefix = pdfout_pdf_to_utf8 (ctx, pdf_buf,
						       pdf_buf_len,
						       &utf8_buf_len);
		}
	    }
	}
      
      pdfout_page_labels_push (labels, &mapping);
      free (mapping.prefix);
    }
  
  return broken;
}

int
pdfout_page_labels_get (pdfout_page_labels_t **labels, fz_context *ctx,
			pdf_document *doc)
{
  int rv;
  *labels = pdfout_page_labels_new ();
  rv = page_labels_get (*labels, ctx, doc);
  if (pdfout_page_labels_length (*labels) == 0)
    {
      pdfout_page_labels_free (*labels);
      *labels = NULL;
    }
  return rv;
}
