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


/* FIXME: replace pdfout_float_sequence with pdfout_yaml_fz_rect_to_? ? */
/* FIXME: use open_memstream */
static void put_char (char c, char **textbuf,
		      size_t *textbuf_len, size_t *textbuf_pos);

void
pdfout_print_yaml_page (fz_context *ctx, pdf_document *pdf_doc,
			int page_number, yaml_emitter_t *emitter, int mode)
{
  yaml_document_t yaml_doc;
  yaml_document_t *doc = &yaml_doc;
  char buffer[256];
  fz_page *page = fz_load_page (ctx, &pdf_doc->super, page_number);
  fz_text_sheet *text_sheet = fz_new_text_sheet (ctx);
  fz_text_page *text_page = fz_new_text_page (ctx);
  fz_device *dev = fz_new_text_device (ctx, text_sheet, text_page);
  fz_matrix transform = fz_identity; /* FIXME: required?? */
  fz_rect bbox, mbox;
  fz_device *bbox_dev = fz_new_bbox_device (ctx, &bbox);
  int value, mapping, block_n, lines;
  size_t textbuf_len = 1;	/* initial length */
  char *textbuf = xmalloc (textbuf_len);
  size_t textbuf_pos;

  ((pdf_page *) page)->ctm = fz_identity;

  fz_run_page (ctx, page, dev, &transform, NULL);
  fz_run_page (ctx, page, bbox_dev, &transform, NULL);

  pdfout_yaml_document_initialize (doc, NULL, NULL, NULL, 0, 0);

  mapping = pdfout_yaml_document_add_mapping (doc, NULL, 0);

  pdfout_snprintf (buffer, sizeof buffer, "%d", page_number + 1);
  pdfout_mapping_push (doc, mapping, "page", buffer);

  mbox = ((pdf_page *) page)->mediabox;
  value = pdfout_float_sequence (doc, mbox.x0, mbox.y0, mbox.x1, mbox.y1);
  pdfout_mapping_push_id (doc, mapping, "mbox", value);

  value = pdfout_float_sequence (doc, bbox.x0, bbox.y0, bbox.x1, bbox.y1);
  pdfout_mapping_push_id (doc, mapping, "bbox", value);

  lines = pdfout_yaml_document_add_sequence (doc, NULL, 0);
  pdfout_mapping_push_id (doc, mapping, "lines", lines);

  for (block_n = 0; block_n < text_page->len; block_n++)
    {
      switch (text_page->blocks[block_n].type)
	{
	case FZ_PAGE_BLOCK_TEXT:
	  /* FIXME: put the following into separate function?  */
	  {
	    fz_text_block *block = text_page->blocks[block_n].u.text;
	    fz_text_line *line;
	    fz_text_char *ch;
	    char utf[4];
	    int i, n;
	    for (line = block->lines; line < block->lines + block->len;
		 line++)
	      {

		int line_mapping =
		  pdfout_yaml_document_add_mapping (doc, NULL, 0);
		fz_text_span *span;
		int span_sequence;
		textbuf_pos = 0;
		bbox = line->bbox;
		value =
		  pdfout_float_sequence (doc, bbox.x0, bbox.y0, bbox.x1,
					 bbox.y1);
		pdfout_mapping_push_id (doc, line_mapping, "bbox", value);

		pdfout_yaml_document_append_sequence_item (doc, lines,
							   line_mapping);
		if (mode > PDFOUT_TXT_YAML_LINES)
		  {
		    span_sequence =
		      pdfout_yaml_document_add_sequence (doc, NULL, 0);
		    pdfout_mapping_push_id (doc, line_mapping, "spans",
					    span_sequence);
		  }
		for (span = line->first_span; span; span = span->next)
		  {
		    int span_mapping;
		    if (mode > PDFOUT_TXT_YAML_LINES)
		      {
			span_mapping =
			  pdfout_yaml_document_add_mapping (doc, NULL, 0);
			bbox = span->bbox;
			value =
			  pdfout_float_sequence (doc, bbox.x0, bbox.y0,
						 bbox.x1, bbox.y1);
			pdfout_mapping_push_id (doc, span_mapping, "bbox",
						value);

			pdfout_yaml_document_append_sequence_item
			  (doc, span_sequence, span_mapping);
			
			textbuf_pos = 0;
		      }
		    if (mode == PDFOUT_TXT_YAML_LINES
			&& span != line->first_span)
		      put_char (' ', &textbuf, &textbuf_len,
				&textbuf_pos);

		    for (ch = span->text; ch < span->text + span->len; ch++)
		      {

			n = fz_runetochar (utf, ch->c);
			for (i = 0; i < n; i++)
			  put_char (utf[i], &textbuf, &textbuf_len,
				    &textbuf_pos);

		      }
		    if (mode > PDFOUT_TXT_YAML_LINES)
		      {
			textbuf[textbuf_pos] = '\0';
			pdfout_mapping_push (doc, span_mapping, "text",
					     textbuf);
		      }

		  }
		if (mode == PDFOUT_TXT_YAML_LINES)
		  {
		    textbuf[textbuf_pos] = '\0';
		    pdfout_mapping_push (doc, line_mapping, "text", textbuf);
		  }

	      }
	    break;
	  }
	case FZ_PAGE_BLOCK_IMAGE:
	  break;
	default:
	  abort ();
	}
    }
  pdfout_yaml_emitter_dump (emitter, doc);

  /* cleanup */
  free (textbuf);
  fz_drop_page (ctx, page);
  fz_drop_device (ctx, dev);
  fz_drop_device (ctx, bbox_dev);
  fz_drop_text_page (ctx, text_page);
  fz_drop_text_sheet (ctx, text_sheet);
}

static void
put_char (char c, char **textbuf, size_t *textbuf_len,
	  size_t *textbuf_pos)
{
  if (*textbuf_pos + 2 > *textbuf_len)
    *textbuf = x2nrealloc (*textbuf, textbuf_len, 1);

  (*textbuf)[(*textbuf_pos)++] = c;
}


static void process_block (FILE *memstream, fz_text_block *block)
{
  fz_text_line *line;

  for (line = block->lines; line - block->lines < block->len;
       line++)
    {
      fz_text_span *span;
      for (span = line->first_span; span; span = span->next)
	{
	  fz_text_char *ch;
	    
	  if (span != line->first_span)
	      putc (' ', memstream);
	  for (ch = span->text; ch < span->text + span->len; ch++)
	    {
	      char utf[4];
	      int len = fz_runetochar (utf, ch->c);
	      if (utf[0] == '\0')
		{
		  pdfout_msg ("process_block: skipping null character");
		  continue;
		}
	      fwrite (utf, len, 1, memstream);
	    }
	}
      putc ('\n', memstream);
    }
}

void
pdfout_text_get_page (FILE *stream, fz_context *ctx,
		      pdf_document *doc, int page_number)
{
  fz_page *page;
  fz_text_sheet *text_sheet;
  fz_text_page *text_page;
  fz_device *dev;
  fz_page_block *block;

  page = fz_load_page (ctx, &doc->super, page_number);
  ((pdf_page *) page)->ctm = fz_identity;
  
  text_sheet = fz_new_text_sheet (ctx);
  text_page = fz_new_text_page (ctx);
  dev = fz_new_text_device (ctx, text_sheet, text_page);
  
  fz_run_page (ctx, page, dev, &fz_identity, NULL);

  for (block = text_page->blocks; block - text_page->blocks < text_page->len;
       ++block)
    if (block->type == FZ_PAGE_BLOCK_TEXT)
      process_block (stream, block->u.text);
  
  fprintf (stream, "\f\n");

  /* cleanup */
  fz_drop_text_page (ctx, text_page);
  fz_drop_text_sheet (ctx, text_sheet);
  fz_drop_device (ctx, dev);
  fz_drop_page (ctx, page);
}
