#include "test.h"
#include "../page-labels.h"
#include "../libyaml-wrappers.h"

#define FREE_LABELS(labels) pdfout_page_labels_free (labels)


void
pdfout_command_page_labels_test (_GL_UNUSED int argc, _GL_UNUSED char **argv)
{
  char *output;
  pdfout_page_labels_t *labels;
  yaml_emitter_t *emitter = test_new_emitter (&output);
  yaml_parser_t *parser;

					   
  {
    parser =  test_new_parser ("[ \
{page: 1}, \
{page: 2, style: roman, prefix: Cover, start: 2}, \
{page: 4, style: Roman}, \
{page: 5, style: letters}, \
{page: 7, style: Letters}, \
{page: 10, style: arabic} \
]");

    test_ok (pdfout_page_labels_from_yaml (&labels, parser));

    test_reset_emitter_output (&output);
    test_ok (pdfout_page_labels_to_yaml (emitter, labels));
  
    test_streq (output, "\
-   page: 1\n\
-   page: 2\n\
    style: roman\n\
    start: 2\n\
    prefix: Cover\n\
-   page: 4\n\
    style: Roman\n\
-   page: 5\n\
    style: letters\n\
-   page: 7\n\
    style: Letters\n\
-   page: 10\n\
    style: arabic\n");

    /* Print labels.  */
    {
      const char **p, *expected[] = {"", "Coverii", "Coveriii", "I", "a", "b",
				     "A", "B", "C", "1", "2", NULL};
      size_t lengthp = sizeof "Coveriii";
      char *resultbuf = xcharalloc (lengthp);
      for (p = expected; *p; ++p)
	{
	  test_assert (resultbuf
		       == pdfout_page_labels_print (p - expected, labels,
						    resultbuf, &lengthp));
	  test_streq (resultbuf, *p);
	}
      free (resultbuf);
    }
    FREE_LABELS (labels);
  }
  
  {
   /* Empty YAML input. */
   test_reset_parser_input (parser, "");
   test_ok (pdfout_page_labels_from_yaml (&labels, parser));
   test_assert (labels == NULL);
 }

 {
   /* Invalid YAML input.  */
   /* Always include the 'prefix' key to make sure it gets freed on error.  */
   const char *broken_input[] = {
     "abc",		 	             /* not a sequence */
     "[abc]",			             /* not a mapping */
     "[{page: 1}, abc]",                     /* abc not a mapping */
     "[{page: 1}, {prefix: [abc]}]",         /* prefix not a scalar */
     "[{prefix: abcde}]",                      /* missing key 'page' */
     "[{page: 0}, {prefix: abc}]",           /* page < 1 */
     "[{page: 2}, {prefix: abc}]",           /* first page != 1 */
     "[{page: 1}, {page: 2}, {page: 1}, {prefix: abc}]", /* pages not
							    increasing */
     "[{page: 1}, {page: 2147483648}, {prefix: abc}]", /* integer overflow */
     NULL};
     
   const char **input;
   for (input = broken_input; *input; ++input)
     {
       test_reset_parser_input (parser, *input);
       test_is (1, pdfout_page_labels_from_yaml (&labels, parser));
     }
 }

 {
   /* Roman numerals. */
   struct test {
     int value;
     const char *expected;
   } *p, tests[] = {
     {1, "i"}, {11, "xi"}, {111, "cxi"}, {1111, "mcxi"},
     {2222, "mmccxxii"}, {0}};
   pdfout_page_labels_mapping_t mapping = {0};
   size_t lengthp = sizeof "mmccxxii";
   char *resultbuf = xcharalloc (lengthp);
   
   mapping.style = PDFOUT_PAGE_LABELS_LOWER_ROMAN;
   labels = pdfout_page_labels_new ();
   test_ok (pdfout_page_labels_push (labels, &mapping));
   for (p = tests; p->value; ++p)
     {
       test_assert (resultbuf
		    == pdfout_page_labels_print (p->value - 1, labels,
						 resultbuf, &lengthp));
       test_streq (resultbuf, p->expected);
     }
   free (resultbuf);
   FREE_LABELS (labels);
 }

 {
   /* Resultbuf allocation of pdfout_page_labels_print.  */
   pdfout_page_labels_mapping_t mapping = {0};
   size_t lengthp = 0;
   char *resultbuf;
   
#define TEST(PAGE, STYLE, PREFIX, EXPECTED)				\
   do									\
     {									\
       mapping.page = PAGE;						\
       mapping.style = STYLE;						\
       mapping.prefix = (char *) PREFIX;				\
									\
       test_ok (pdfout_page_labels_push (labels, &mapping));		\
       resultbuf = (char *) 1;						\
       lengthp = 0;							\
       resultbuf = pdfout_page_labels_print (PAGE, labels, resultbuf,	\
					     &lengthp);			\
									\
       test_assert (lengthp == sizeof EXPECTED);			\
       test_streq (resultbuf, EXPECTED);				\
       free (resultbuf);						\
									\
       lengthp = 0;							\
									\
       resultbuf = pdfout_page_labels_print (PAGE, labels, NULL,	\
					     &lengthp);			\
       test_assert (lengthp == sizeof EXPECTED);			\
       test_streq (resultbuf, EXPECTED);				\
       free (resultbuf);						\
     }									\
   while (0)

   labels = pdfout_page_labels_new ();
   
   TEST (0, PDFOUT_PAGE_LABELS_NO_NUMBERING, NULL, "");
   TEST (1, PDFOUT_PAGE_LABELS_NO_NUMBERING, "abc", "abc");
   TEST (2, PDFOUT_PAGE_LABELS_ARABIC, NULL, "1");
   TEST (3, PDFOUT_PAGE_LABELS_UPPER, NULL, "A");
   TEST (4, PDFOUT_PAGE_LABELS_UPPER_ROMAN, NULL, "I");
   TEST (5, PDFOUT_PAGE_LABELS_ARABIC, "abc", "abc1");
   
   FREE_LABELS (labels);
 }
   
 /* TODO: set/get tests, page count test... */
 
yaml_parser_free (parser);
yaml_emitter_free (emitter);
exit (0);
}
