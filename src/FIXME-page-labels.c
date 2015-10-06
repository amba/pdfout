#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <yaml.h>
#include <exitfail.h>
#include <xalloc.h>
/* static const char *event_names[] = { */
/*   "YAML_NO_EVENT","YAML_STREAM_START_EVENT","YAML_STREAM_END_EVENT", */
/*   "YAML_DOCUMENT_START_EVENT","YAML_DOCUMENT_END_EVENT","YAML_ALIAS_EVENT", */
/*   "YAML_SCALAR_EVENT","YAML_SEQUENCE_START_EVENT","YAML_SEQUENCE_END_EVENT", */
/*   "YAML_MAPPING_START_EVENT","YAML_MAPPING_END_EVENT" */
/* }; */

static bool yaml_is_scalar (yaml_event_t *event)
{
  assert (event);
  return event->type == YAML_SCALAR_EVENT;
}

static char *yaml_scalar_value (yaml_event_t *event)
{
  assert (yaml_is_scalar (event));
  return (char *) event->data.scalar.value;
}
    
static bool yaml_is_sequence_start (yaml_event_t *event)
{
  assert (event);
  return event->type == YAML_SEQUENCE_START_EVENT;
}
static bool yaml_is_sequence_end (yaml_event_t *event)
{
  assert (event);
  return event->type == YAML_SEQUENCE_END_EVENT;
}
static bool yaml_is_mapping_start (yaml_event_t *event)
{
  assert (event);
  return event->type == YAML_MAPPING_START_EVENT;
}
static bool yaml_is_mapping_end (yaml_event_t *event)
{
  assert (event);
  return event->type == YAML_MAPPING_END_EVENT;
}

#define MSG(fmt, args...)						\
  error ( 0, 0, "converting YAML to page labels: " fmt, ## args)
  
static int
parse (yaml_parser_t *parser, yaml_event_t *event)
{
  yaml_event_delete (event);
  if (yaml_parser_parse (parser, event) == 0)
    {
      MSG ("%s", parser->problem);
      return 1;
    }
  return 0;
}

/* Parse stream/document start events.
   Return values:
   0: EVENT points to the document start event.
   1: empty document.
   2: error. errno is set.  */
int yaml_parser_start (yaml_parser_t *parser, yaml_event_t *event)
{
  /* FIXME! check that encoding is utf8? allow utf16? */
  if (parse (parser, event))
    return 2;
  
  assert (event->type == YAML_STREAM_START_EVENT);
  
  if (parse (parser, event))
    return 2;
  
  if (event->type == YAML_STREAM_END_EVENT)
    /* Empty doc.  */
    return 1;

  assert (event->type == YAML_DOCUMENT_START_EVENT);
  return 0;
}

enum labels_style_e {
  PDFOUT_PAGE_LABELS_NO_NUMBERING,
  PDFOUT_PAGE_LABELS_ARABIC,
  PDFOUT_PAGE_LABELS_UPPER_ROMAN,
  PDFOUT_PAGE_LABELS_LOWER_ROMAN,
  PDFOUT_PAGE_LABELS_UPPER,
  PDFOUT_PAGE_LABELS_LOWER
};
typedef enum labels_style_e labels_style_t;

typedef struct page_labels_mapping {
  int page;	       /*  labels[0].page must be 0.  */
  labels_style_t style;
  char *prefix;	       /* Can be null.  */
  int start;	       /* If 0, use default value of 1.  */
} page_labels_mapping_t;
  
typedef struct page_labels {
  /* See section 12.4.2 in PDF 1.7 reference.  */
  int len, cap; 			/* >= 1.  */
  page_labels_mapping_t *labels;
} page_labels_t;

page_labels_t *
labels_create (void)
{
  page_labels_t *labels = XZALLOC (page_labels_t);
  return labels;
}

void labels_free (page_labels_t *labels)
{
  if (labels)
    {
      free (labels->labels);
      free (labels);
    }
}

void labels_x2nrealloc (page_labels_t *labels)
{
  size_t cap = labels->cap;
  labels->labels = x2nrealloc (labels->labels, &cap,
			       sizeof (page_labels_mapping_t));
  if (cap >= INT_MAX)
    error (exit_failure, 0, "INT_MAX overflow");
  labels->cap = cap;
}

int
strtoui (const char *s)
{
  char *tailptr;
  long rv = strtol (s, &tailptr, 10);
  if (rv < 0 || tailptr[0] != 0 || tailptr == s)
    {
      MSG ("not a positive int: '%s'", s);
      return -1;
    }
  if (rv > INT_MAX)
    {
      MSG ("integer overflow: '%s'", s);
      return -1;
    }
  return rv;
}

/* Return value:
   0: *KEY and *VALUE are allocated.
   1: Encountered a mapping end event.
   2: Error.  */

int
get_scalar_key_value_pair (char **key, char **value,
			   yaml_parser_t *parser, yaml_event_t *event)
{
  char *key_value[2];
  int i;
  
  for (i = 0; i < 2; ++i)
    key_value[i] = NULL;
  
  for (i = 0; i < 2; ++i)
    {
      if (parse (parser, event))
	goto error;
      
      if (yaml_is_mapping_end (event))
	goto end_event;
      
      if (yaml_is_scalar (event) == false)
	{
	  MSG ("expected scalar");
	  goto error;
	}
      key_value[i] = xstrdup (yaml_scalar_value (event));
    }

      *key = key_value[0];
      *value = key_value[1];
  
      return 0;
    end_event:
      for (i = 0; i < 2; ++i)
	free (key_value[i]);
      MSG ("get_scalar_key_value_pair: found mapping end event");
      return 1;
      
    error:
      for (i = 0; i < 2; ++i)
	free (key_value[i]);
      return 2;
}
 
static int process_key_value_pair (char *key, char *value,
				   page_labels_t *labels)
{
  int page;
  page_labels_mapping_t *mapping = &labels->labels[labels->len];
  
  MSG ("process_key_value_pair: key: %s, value: %s", key, value);
  if (strcasecmp (key, "page") == 0)
    {
      page = mapping->page = strtoui (value);
      
      if (page == -1)
	return 1;
      
      if (page < 1)
	{
	  MSG ("page '%s' must be > 0", value);
	  return 1;
	}
      if (labels->len == 0 && page != 1)
	{
	  MSG ("page '%s' for first label must be 1", value);
	  return 1;
	}
    }
  return 0;
}
				   
static int
get_page_labels_mapping (page_labels_t *labels, yaml_parser_t *parser,
			 yaml_event_t *event)
{
  char *key , *value;
  int rv, rv_key_val;
  MSG ("get_page_labels_mapping: labels->len = %d", i);

  if (labels->len == labels->cap)
    labels_x2nrealloc (labels);
  
  page_labels_mapping_t *mapping = &labels->labels[labels->len];
  
  while (1)
    {
      rv = get_scalar_key_value_pair (&key, &value, parser, event);
      if (rv == 0)
	{
	  rv_key_val = process_key_value_pair (key, value, labels);
	  free (key, value);
	  if (rv_key_val)
	    return 1;
	}
      else if (rv == 2)
	return 1;
      else if (rv == 1)
	{
	  /* Reached end of mapping.  */
	  if (labels->page == 0)
	    {
	      MSG ("missing key 'page'");
	      goto error;
	    }  
	  /* Populate page labels mapping.  */
	  labels->labels[i].page = page - 1;
	  ++labels->len;
	  
	  break;
	}
      else
	
	  
    }

  return 0;
    }

/* Return 0 for success, 1 for empty sequence and 2 on error.  */
static int
get_page_labels_sequence (page_labels_t *labels, yaml_parser_t *parser,
			  yaml_event_t *event)
{
  bool have_mapping = false;
  while (1)
    {
      if (parse (parser, event))
	return 2;
      if (yaml_is_mapping_start (event))
	{
	  MSG ("get_page_labels_sequence: labels->len: %d", labels->len);
	  if (get_page_labels_mapping (labels, parser, event))
	    return 2;
	  else
	    have_mapping = true;
	}
      else if (yaml_is_sequence_end (event))
	break;
      else
	{
	  MSG ("unexpected event");
	  return 2;
	}
    }

  if (have_mapping == false)
    return 1;
  
  return 0;
}
 
static int
get_page_labels (page_labels_t **labels_ptr, yaml_parser_t *parser,
		 yaml_event_t *event)
{
  page_labels_t *labels;
  int rv = 0;

  if (parse (parser, event))
    return 1;
  
  if (yaml_is_sequence_start (event) == false)
    {
      MSG ("expected sequence start event");
      return 1;
    }
  
  labels = *labels_ptr = labels_create ();
  
  rv = get_page_labels_sequence (labels, parser, event);
  
  if (rv == 1)
    {
      labels_free (labels);
      *labels_ptr = NULL;
    }
  else if (rv == 2)
    {
      labels_free (labels);
      return 1;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  yaml_parser_t *parser = XMALLOC (yaml_parser_t);
  /* XZALLOC, so that 'yaml_event_delete (event)' is legal.  */
  yaml_event_t *event = XZALLOC (yaml_event_t);
  
  if (!yaml_parser_initialize (parser))
    error (1, errno, "error");
  
  FILE *file = fopen ("/tmp/yaml", "r");
  if (!file)
    error (1, errno, "open");
  yaml_parser_set_input_file (parser, file);
  
  int rv = yaml_parser_start (parser, event);
  if (rv == 1)
    error (1, 0, "empty doc");
  else if (rv == 2)
    error (1, errno, "parsing error");
  page_labels_t *labels;

  if (get_page_labels (&labels, parser, event))
    MSG ("get_page_labels");
  else if (labels == NULL)
    MSG ("get_page_labels: NULL");
  else
    MSG ("len: %d", labels->len);

  labels_free (labels);
  yaml_parser_delete (parser);
  free (parser);
  free (event);
  return 0;
}
