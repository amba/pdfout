#ifndef PDFOUT_ARGP_SHARED_H
#define PDFOUT_ARGP_SHARED_H
#include "../common.h"

/* command function prototypes  */
typedef void command_func_t (fz_context *ctx, int argc, char **argv);
#define DEF_COMMAND(name, type, function, description)	\
  command_func_t function;
#include "commands.def"
#undef DEF_COMMAND

extern char *pdfout_program_name;

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

int strmatch (const char *key, const char *const *list);
  
/* Calls 'exit (exit_failure)' on error.  */
fz_context *pdfout_new_context (void);

void pdfout_ensure_binary_io (fz_context *ctx);
  
/* Append SUFFIX to FILENAME and call fopen on the resulting filename. */
FILE *open_default_read_file (fz_context *ctx, const char *filename,
			      const char *suffix);

FILE *open_default_write_file (fz_context *ctx, const char *filename,
			       const char *suffix);

char * pdfout_strsep (fz_context *ctx, char **string_ptr, char delimiter);

int *pdfout_parse_page_range (fz_context *ctx, const char *range,
			      int page_count);

#define PDFOUT_VERSION \
"pdfout 0.1\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\
\nThis is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law."

#endif
