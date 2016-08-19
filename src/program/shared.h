#ifndef PDFOUT_ARGP_SHARED_H
#define PDFOUT_ARGP_SHARED_H
#include "../common.h"

/* command function prototypes  */
typedef void command_func_t (fz_context *ctx, int argc, char **argv);
#define DEF_COMMAND(name, type, function, description)	\
  command_func_t function;
#include "commands.def"
#undef DEF_COMMAND

/* first value for option enums to be used in commands */
#define PDFOUT_ARGP_PRIVATE_ENUM = INT_MAX / 2

#define PDFOUT_NO_INCREMENTAL "Write modified document to FILE"

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define pdfout_no_output_msg() pdfout_msg ("no output written")

#define pdfout_output_to_msg(filename)				\
  do								\
    {								\
      if (filename)						\
	pdfout_msg ("output written to '%s'", filename);	\
    } while (0)

/* general options */
struct argp pdfout_general_argp;

struct argp pdfout_pdf_output_argp;

/* YAML emitter options */
struct argp pdfout_yaml_emitter_argp;

/* Call c_tolower on each byte of S.  */
char *lowercase (char *s);

/* Call c_toupper on each byte of S.  */
char *upcase (char *s);
  
int strmatch (const char *key, const char *const *list);
  
/* Calls argp_failure on error.  */
FILE *xfopen (struct argp_state *state, const char *path, const char *mode);

/* Calls 'exit (exit_failure)' on error.  */
fz_context *pdfout_new_context (void);

void pdfout_argp_parse (const struct argp *argp, int argc, char ** argv,
			unsigned flags, int *arg_index, void *input);

/* Append SUFFIX to FILENAME and call fopen on the resulting filename. */
FILE *open_default_read_file (fz_context *ctx, const char *filename,
			      const char *suffix);

FILE *open_default_write_file (fz_context *ctx, const char *filename,
			       const char *suffix);

/* calls exit (EX_USAGE) on error */
enum pdfout_outline_format pdfout_outline_get_format (struct argp_state *state,
						      char *format);

int *pdfout_parse_page_range (fz_context *ctx, const char *range,
			      int page_count);

#define PDFOUT_OUTLINE_FORMATS "YAML|WYSIWYG"
#define PDFOUT_OUTLINE_FORMAT_OPTION \
  "format", 'f', PDFOUT_OUTLINE_FORMATS, 0, \
"outline format, default: YAML, case-insensitive"

#define PDFOUT_VERSION \
  PACKAGE_STRING "\
\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\
\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law."

#endif	/* !PDFOUT_ARGP_SHARED_H */
