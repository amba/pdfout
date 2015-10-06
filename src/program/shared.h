#ifndef PDFOUT_ARGP_SHARED_H
#define PDFOUT_ARGP_SHARED_H
#include "../common.h"
#include "libyaml-wrappers.h"
#include <exitfail.h>

/* command function prototypes  */
typedef void command_func_t (int argc, char **argv);
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
  
/* Lowercase arg and run argmatch.  */
ptrdiff_t argcasematch (char *arg, const char *const *valid);
  
/* calls exit (EX_USAGE) on failure */
FILE *pdfout_xfopen (const char *path, const char *mode);

/* calls exit (1) on error */
fz_context *pdfout_new_context (void);

void pdfout_argp_parse (const struct argp * argp, int argc, char ** argv,
			unsigned flags, int *arg_index, void *input);

/* calls pdfout_xfopen to open a stream */
FILE *pdfout_get_stream (char **output_filename, char mode,
			 const char *pdf_filename,
			 bool use_default_filename, const char *suffix);

/* calls exit (EX_USAGE) on error */
enum pdfout_outline_format pdfout_outline_get_format (struct argp_state *state,
						      char *format);

/* calls exit (EX_USAGE) on error */
int *pdfout_parse_page_range (const char *range, int page_count);

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
