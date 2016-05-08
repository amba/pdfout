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

static void process_block (FILE *memstream, fz_stext_block *block)
{
  fz_stext_line *line;
  /* printf ("in process block, block->len = %d\n", block->len); */
  for (line = block->lines; line - block->lines < block->len;
       line++)
    {
      fz_stext_span *span;
      for (span = line->first_span; span; span = span->next)
	{
	  fz_stext_char *ch;
	    
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
  fz_stext_sheet *sheet;
  fz_stext_page *text;

  sheet = fz_new_stext_sheet (ctx);

  text = fz_new_stext_page_from_page_number (ctx, &doc->super, page_number,
					     sheet);
  
  /* printf ("in pdfout_text_get_page, page_number: %d, page->len: %d\n", */
  /* 	  page_number, text->len); */

  for (int i = 0; i < text->len; ++i)
    {
      fz_page_block *block = &text->blocks[i];
      if (block->type == FZ_PAGE_BLOCK_TEXT)
	process_block (stream, block->u.text);
    }
  
  fprintf (stream, "\f\n");

  /* cleanup */
  fz_drop_stext_page (ctx, text);
  fz_drop_stext_sheet (ctx, sheet);
}
