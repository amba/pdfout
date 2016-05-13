/* The pdfout document modification and analysis tool.
   Copyright (C) 2015 AUTHORS (see AUTHORS file)
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */


#include "common.h"

#include "outline-wysiwyg.h"
#include "pdfout-regex.h"

#include <errno.h>
#include <xmemdup0.h>
#include <xalloc.h>
#include <unistr.h>
#include <limits.h>


typedef struct line_entry
{
  char *buffer;
  int len;
} *line_entry_t;

typedef struct bookmark_s
{
  char *title;
  int title_len;
  int page;
  int level;
} bookmark;




static int
get_line_table (line_entry_t *line_table, FILE *infile)
{
  char *line = NULL;
  size_t line_len = 0;
  size_t line_table_length = 1;
  size_t i;
  ssize_t read;

  *line_table = XNMALLOC (line_table_length, struct line_entry);

  i = 0;
  errno = 0;
  while ((read = getline (&line, &line_len, infile)) != -1)
    {
      if (i + 2 > line_table_length)
	*line_table = x2nrealloc (*line_table, &line_table_length,
				  sizeof (struct line_entry));
      if (read >= INT_MAX)
	error (1, 0, "integer overflow");

      /* remove trailing newline */
      if (read > 0 && line[read - 1] == '\n')
	--read;
      (*line_table)[i].buffer = xmalloc (read + 1);
      (*line_table)[i].len = read;
      memcpy ((*line_table)[i].buffer, line, read);
      (*line_table)[i].buffer[read] = '\0';
      ++i;
    }
  
  /* catch allocation error in getline */
  if (errno)
    error (1, errno, "get_line_table: allocation error in getline");

  (*line_table)[i].buffer = NULL;
  (*line_table)[i].len = 0;
  free (line);
  return i;
}

static struct pdfout_re_pattern_buffer *pattern_buffer[2];

static void
setup_regexps (void)
{
#define BLANK "[ \t]"
#define SEPARATOR "[ \t.]"
#define NUMBER "(-?[0-9]+)"
  int i;
  const char *error_string;
  const char *regex[2] = {
    SEPARATOR "+"  NUMBER  BLANK "*" "$",
    "d=" NUMBER BLANK "*" "$"  /* offset number */
  };

  if (pattern_buffer[0])
    /* already setup */
    return;

  for (i = 0; i < 2; ++i)
    {
      pattern_buffer[i] = XZALLOC (struct pdfout_re_pattern_buffer);
      error_string =
	pdfout_re_compile_pattern (regex[i], strlen (regex[i]),
				   RE_SYNTAX_POSIX_EXTENDED,
				   0, pattern_buffer[i]);
      if (error_string)
	error (1, errno, "setup outline-wysiwyg regex: %s", error_string);
    }
}

#define MSG(fmt, args...) pdfout_msg ("parsing WYSIWYG format: " fmt, ## args)

static int
line_table_to_bookmarks (bookmark *bookmarks, int num_lines,
			 line_entry_t lines)
{
  int i, bm_num, offset;

  setup_regexps ();

  bm_num = 0;
  offset = 0;

  for (i = 0; i < num_lines; ++i)
    {
      char *check, *line = lines[i].buffer;
      
      int error_code, indent, spaces, blanks, page, len = lines[i].len;
      bookmark *bm;

      /* check UTF-8 integrity */
      check = (char *) u8_check ((const uint8_t *) line, len);
      if (check)
	{
	  /* FIXME!: use proper pointer format specifier, not %lu.  */
	  MSG ("invalid UTF-8 sequence in input line %d at column %lu", i + 1,
	       check - line);
	  return 1;
	}
	       
      /* find leading spaces */
      spaces = strspn (line, " ");

      /* round down to multiple of four */
      indent = spaces - spaces % 4;
      
      /* check whether line is blank */
      blanks = spaces + strspn (line + spaces, " \t");
      
      /* skip blank lines */
      if (blanks == len)
	continue;
      
      error_code = pdfout_re_search (pattern_buffer[0], line, len, indent,
				     len);

      if (error_code == -2)
	/* internal error */
	error (1, errno, "re_search");
      
      else if (error_code == -1)
	{
	  /* no match. check for page offset marker */
	  error_code = pdfout_re_match (pattern_buffer[1], line ,len, blanks);
	  if (error_code == -2)
	    /* internal error */
	    error (1, errno, "re_match");
	  else if (error_code == -1)
	    {
	      MSG ("input line %d is invalid:\n%s", i + 1, line);
	      return 1;
	    }
	  /* match */
	  offset = pdfout_strtoint (line + pattern_buffer[1]->regs.start[1],
				    NULL);
	  if (offset > INT_MAX / 10 || offset < INT_MIN / 10)
	    {
	      MSG ("overflow in input line %d:\n%s", i + 1, line);
	      return 1;
	    }
	  continue;
	}
	
      /* match */
      bm = bookmarks + bm_num;
      bm->level = indent / 4;

      /* level increment > 1 ? */
      if ((bm_num > 0 && bm->level > bookmarks[bm_num - 1].level + 1)
	  || (bm_num == 0 && bm->level > 0))
	{
	  MSG ("too much indentation in line %d", i + 1);
	  return 1;
	}

      bm->title_len = pattern_buffer[0]->regs.start[0] - indent;
	  
      bm->title = xmemdup0 (line + indent, bm->title_len);

      page = pdfout_strtoint (line + pattern_buffer[0]->regs.start[1], NULL);
      if (page > INT_MAX / 10 || page < INT_MIN / 10)
	{
	  MSG ("overflow in input line %d:\n%s", i + 1, line);
	  return 1;
	}
      
      bm->page = offset + page;
      
      if (bm->page < 1)
	{
	  MSG ("input line %d: page number %d < 1", i + 1, bm->page);
	  return 1;
	}
	  
      ++bm_num;
    }
  
  return 0;
}

static int
get_yaml_sequence (yaml_document_t *doc, bookmark *bookmarks,
		   int first, int *next)
{
  int i = first;
  bookmark bm = bookmarks[i];
  int level = bm.level;

  int sequence = pdfout_yaml_document_add_sequence (doc, NULL, 0);
  int mapping, kids;
  char buffer[256];

  while (1)
    {
      bm = bookmarks[i];
      if (bm.title == NULL)
	break;
      assert (bm.level <= level);
      if (bm.level < level)
	break;
      mapping = pdfout_yaml_document_add_mapping (doc, NULL, 0);
      pdfout_yaml_document_append_sequence_item (doc, sequence, mapping);
      pdfout_mapping_push (doc, mapping, "title", bm.title);

      pdfout_snprintf_old (buffer, sizeof buffer, "%d", bm.page);
      pdfout_mapping_push (doc, mapping, "page", buffer);

      if (bookmarks[i + 1].level > bm.level)
	{
	  kids = get_yaml_sequence (doc, bookmarks, i + 1, &i);
	  pdfout_mapping_push_id (doc, mapping, "kids", kids);
	}
      else
	++i;
    }
  *next = i;
  return sequence;
}

static void
free_line_table (line_entry_t table)
{
  int i = 0;

  while (table[i].buffer)
    {
      free (table[i].buffer);
      ++i;
    }

  free (table);
}

static void
free_bookmarks (bookmark *bookmarks)
{
  int i = 0;

  while (bookmarks[i].title)
    {
      free (bookmarks[i].title);
      ++i;
    }
  free (bookmarks);
}

int
pdfout_outline_wysiwyg_to_yaml (yaml_document_t **doc, FILE *infile)
{
  line_entry_t line_table;
  bookmark *bookmarks;
  int num_lines, dummy;
  bool failed = false;

  *doc = XZALLOC (yaml_document_t);
  pdfout_yaml_document_initialize (*doc, NULL, NULL, NULL, 1, 1);

  num_lines = get_line_table (&line_table, infile);
  bookmarks = XCALLOC (num_lines + 1, bookmark);

  if (line_table_to_bookmarks (bookmarks, num_lines, line_table))
    failed = true;
  else
    get_yaml_sequence (*doc, bookmarks, 0, &dummy);

  free_line_table (line_table);
  free_bookmarks (bookmarks);
  
  if (failed)
    {
      yaml_document_delete (*doc);
      free (*doc);
      return 1;
    }

  return 0;
}


/* YAML to WYSIWYG format conversion */

#undef MSG
#define MSG(fmt, args...) \
  pdfout_msg ("convert YAML outline to wysiwyg format: " fmt, ## args)

static int
yaml_sequence_node_to_wysiwyg (yaml_document_t *doc, int sequence, int level,
			      FILE *output)
{
  yaml_node_t *node;
  char *title, *page, *endptr;
  size_t title_len;
  int length, i, j, mapping, kids, page_test;
  node = yaml_document_get_node (doc, sequence);
  if (node == NULL || node->type != YAML_SEQUENCE_NODE)
    {
      MSG ("not a sequence node");
      return 1;
    }
  length = pdfout_sequence_length (doc, sequence);
  for (i = 0; i < length; ++i)
    {
      mapping = pdfout_sequence_get (doc, sequence, i);
      node = yaml_document_get_node (doc, mapping);
      assert (node);
      if (node->type != YAML_MAPPING_NODE)
	{
	  MSG ("not a mapping node");
	  return 1;
	}
      node = pdfout_mapping_gets_node (doc, mapping, "title");
      if (node == NULL || node->type != YAML_SCALAR_NODE)
	{
	  MSG ("missing key 'title'");
	  return 1;
	}
      title = pdfout_scalar_value (node);
      if (title[0] == ' ')
	MSG ("trimming leading blanks for title '%s'", title);
      while (1)
	{
	  if (title[0] != ' ')
	    break;
	  ++title;
	}
      title_len = strlen (title);

      if (title_len >= 1 && (title[title_len - 1] == '.'
			     || title[title_len - 1] == ' '))
	{
	  MSG ("trailing dots or whitespace will get lost for title '%s'",
	       title);
	}
      
      node = pdfout_mapping_gets_node (doc, mapping, "page");
      if (node == NULL || node->type != YAML_SCALAR_NODE)
	{
	  MSG ("missing key 'page' for title '%s'", title);
	  return 1;
	}
      page = pdfout_scalar_value (node);
      page_test = pdfout_strtoint (page, &endptr);
      if (page_test <= 0 || endptr == page || endptr[0] != '\0')
	{
	  MSG ("invalid page '%s' for title '%s'", page, title);
	  return 1;
	}

      for (j = 0; j < level * 4; ++j)
	fputc (' ', output);
      fprintf (output, "%s %s\n", title, page);

      kids = pdfout_mapping_gets (doc, mapping, "kids");
      if (kids)
	if (yaml_sequence_node_to_wysiwyg (doc, kids, level + 1, output))
	  return 1;

    }
  return 0;
}

int
pdfout_outline_dump_to_wysiwyg (FILE *output, yaml_document_t *doc)
{
  yaml_node_t *root = yaml_document_get_root_node (doc);
  if (root == NULL)
    {
      MSG ("empty YAML");
      return 0;
    }
  
  return yaml_sequence_node_to_wysiwyg (doc, 1, 0, output);
}
