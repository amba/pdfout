#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>

#include <mupdf/pdf.h>
#include <mupdf/fitz.h>

static pdf_document *
check_open (fz_context * ctx, char *file)
{
  pdf_document *doc = pdf_open_document (ctx, file);
  if (doc->repair_attempted || doc->freeze_updates)
    error (1, 0, "pdf_open_document: broken_pdf: %s", file);
  return doc;
}


int
main (void)
{
  fz_write_options opts = {0};
  fz_context *ctx;
  pdf_document *doc[3];
  pdf_obj *info;
  char *file;
  char *subject = "check incremental update with xref";
  int num;

  ctx = fz_new_context (NULL, NULL, FZ_STORE_DEFAULT);
  
  file = tmpnam (NULL);
  error (0, 0, "file: %s", file);
  doc[0] = pdf_create_document (ctx);

  opts.do_incremental = 0;
  pdf_write_document (ctx, doc[0], file, &opts);

  doc[1] = check_open (ctx, file);

  /* ensure that incremental update will have xref stream */
  doc[1]->has_xref_streams = 1;

  num = pdf_create_object (ctx, doc[1]);
  info = pdf_new_dict (ctx, doc[1], 1);
  pdf_update_object (ctx, doc[1], num, info);
  pdf_drop_obj (ctx, info);

  pdf_dict_puts_drop (ctx, info, "Subject",
		      pdf_new_string (ctx, doc[1], subject, strlen (subject)));
  pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc[1]), "Info",
		      pdf_new_indirect (ctx, doc[1], num, 0));


  opts.do_incremental = 1;
  pdf_write_document (ctx, doc[1], file, &opts);

  doc[2] = check_open (ctx, file);
  
  exit (0);
}
