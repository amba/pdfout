/*

MuPDF is Copyright 2006-2015 Artifex Software, Inc.

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU Affero General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU Affero General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#include "common.h"

static pdf_obj *
resolve_dest_rec(fz_context *ctx, pdf_document *doc, pdf_obj *dest,
		 fz_link_kind kind, int depth)
{
  if (depth > 10) /* Arbitrary to avoid infinite recursion */
    return NULL;

  if (pdf_is_name(ctx, dest) || pdf_is_string(ctx, dest))
    {
      if (kind == FZ_LINK_GOTO)
	{
	  dest = pdf_lookup_dest(ctx, doc, dest);
	  dest = resolve_dest_rec(ctx, doc, dest, kind, depth+1);
	}

      return dest;
    }

  else if (pdf_is_array(ctx, dest))
    {
      return dest;
    }

  else if (pdf_is_dict(ctx, dest))
    {
      dest = pdf_dict_get(ctx, dest, PDF_NAME_D);
      return resolve_dest_rec(ctx, doc, dest, kind, depth+1);
    }

  else if (pdf_is_indirect(ctx, dest))
    return dest;

  return NULL;
}

pdf_obj *
pdfout_resolve_dest(fz_context *ctx, pdf_document *doc, pdf_obj *dest,
		    fz_link_kind kind)
{
  return resolve_dest_rec(ctx, doc, dest, kind, 0);
}
