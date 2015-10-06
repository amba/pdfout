#ifndef HAVE_LIBYAML_WRAPPERS_H
#define HAVE_LIBYAML_WRAPPERS_H
#include "../libyaml-wrappers.h"

/* Wrappers around libyaml.
   On error, call exit (exit_failure).  */

/* Parser.  */

/* Allocate, and initialize new parser. Use INPUT, if non-NULL.  */
yaml_parser_t *yaml_parser_new (FILE *input);

/* Free a parser created with yaml_parser_new.  */
void yaml_parser_free (yaml_parser_t *parser);

/* Emitter.  */

/* Allocate and initialize emitter.  */
yaml_emitter_t *yaml_emitter_new (FILE *output);

/* Free emitter created with yaml_emitter_new. */
void yaml_emitter_free (yaml_emitter_t *emitter);



#endif	/* !HAVE_LIBYAML_WRAPPERS_H */
