#include "common.h"
#include "c-ctype.h"

#define THROW(ctx, fmt, args...)				\
  pdfout_throw (ctx, "check date string: " fmt, ## args)

/* Return -1 on error.  */
static int get_number (const char **s, int num_len, int *len)
{
  if (num_len > *len)
    return -1;
  
  int retval = 0;
  for (int i = 0; i < num_len; ++i)
    {
      if (c_isdigit ((*s)[i]))
	retval = retval * 10 + ((*s)[i] - '0');
      else
	return -1;
    }
  *len -= num_len;
  *s += num_len;
  return retval;
}

static void assert_finished (fz_context *ctx, int len)
{
  if (len == 0)							
    return;								
  pdfout_throw (ctx, "trailing garbage in date");	
}

int
pdfout_check_date_string (fz_context *ctx, const char *date)
{
  /* Date is of the form D:YYYYMMDDHHmmSSOHH'mm.  */

  int len = strlen (date);

  if (len >= 2 && !memcmp (date, "D:", 2))
    len -= 2, date += 2;

  /* YEAR */
  if (get_number (&date, 4, &len) < 0)
    pdfout_throw (ctx, "no year in date");

  /* MONTH */
  int month = get_number (&date, 2, &len);
  if (month < 0)
    assert_finished (ctx, len);
  if (month < 1 || month > 12)
    pdfout_throw (ctx, "month ouf of range (01-12)");

  /* DAY */
  int day = get_number (&date, 2, &len);
  if (day < 0)
    assert_finished (ctx, len);
  if (day < 1 || day > 31)
    pdfout_throw (ctx, "day ouf of range (01-31)");

  /* HOUR */
  int hour = get_number (&date, 2, &len);
  if (hour < 0)
    assert_finished (ctx, len);
  if (hour > 23)
    pdfout_throw (ctx, "hour ouf of range (00-23)");

  /* MINUTE */
  int minute = get_number (&date, 2, &len);
  if (minute < 0)
    assert_finished (ctx, len);
  if (minute > 59)
    pdfout_throw (ctx, "minute ouf of range (00-59)");

  /* SECOND */
  int second = get_number (&date, 2, &len);
  if (second < 0)
    assert_finished (ctx, len);
  if (second > 59)
    pdfout_throw (ctx, "second ouf of range (00-59)");


  /* UT offset specifier.  */
  if (len && (date[0] == '+' || date[0] == '-' || date[0] == 'Z'))
    --len, ++date;
  else
    assert_finished (ctx, len);

  /* HOUR offset */
  hour = get_number (&date, 2, &len);
  if (hour < 0)
    assert_finished (ctx, len);
  else
    {
      if (hour > 23)
	pdfout_throw (ctx, "hour offset ouf of range (00-23)");
      
      if (len && date[0] == '\'')
	--len, ++date;
      else
	pdfout_throw (ctx, "missing apostrophe after hour offset");
    }

  /* MINUTE offset */
  minute = get_number (&date, 2, &len);
  if (minute < 0)
    assert_finished (ctx, len);
  if (minute > 59)
    pdfout_throw (ctx, "minute offset ouf of range (00-59)");

  assert_finished (ctx, len);
}
