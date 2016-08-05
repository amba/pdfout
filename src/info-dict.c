#include "common.h"
#include "c-ctype.h"

/* FIXME: test date check function.  */

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

static void
check_date_string (fz_context *ctx, const char *date)
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

static void
check_key_val_pair (fz_context *ctx, const char *name, const char *string,
		    int string_len)
{
  if (!strcmp (name, "CreationDate") || !strcmp (name, "ModDate"))
    {
      check_date_string (ctx, string);
    }
  else if (!strcmp (name, "Trapped"))
    {
      if (strlen (string) != string_len)
	pdfout_throw (ctx, "value of key 'Trapped' has embedded null byte");
      if (strcmp(string, "True") && strcmp (string, "False") &&
	  strcmp (string, "Unknown"))
	{
	  pdfout_throw (ctx, "invalid value '%.*s' of key 'Trapped'.\n"
			"valid values are True, False, Unknown",
			string_len, string);
	}
    }
 else if (strcmp (name, "Title") && strcmp (name, "Author")
	  && strcmp (name, "Subject") && strcmp (name, "Keywords")
	  && strcmp (name, "Creator") && strcmp (name, "Producer"))
   pdfout_throw (ctx, "'%s' is not a valid infodict key. Valid keys are:\n\
Title,Author,Subject,Keywords,Creator,Producer,CreationDate,ModDate,Trapped",
		 name);
}

static void
check_info_dict (fz_context *ctx, pdfout_data *info)
{
  int len = pdfout_data_hash_len (ctx, info);

  for (int i = 0; i < len; ++i)
    {
      char *name, *string;
      int string_len;
      pdfout_data_hash_get_key_value (ctx, info, &name, &string, &string_len,
				      i);
      check_key_val_pair (ctx, name, string, string_len);
    }
  return;
}

static void
insert_key_value (fz_context *ctx, pdf_document *doc, pdf_obj *dict,
		  const char *key, const char *value, int value_len)
{
  pdf_obj *string_obj = pdfout_utf8_to_str_obj (ctx, doc, value, value_len);
  pdf_dict_puts_drop (ctx, dict, key, string_obj);
}

void
pdfout_info_dict_set (fz_context *ctx, pdf_document *doc,
		      pdfout_data *info, bool append)
{
  if (info)
    check_info_dict (ctx, info);

  
  pdf_obj *pdf_info = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Info");
  pdf_obj *info_ref = NULL, *new_info = NULL;

  /* FIXME: do proper try/catch.  */
  
  if (pdf_info == NULL)
    {
      pdf_info = pdf_new_dict (ctx, doc, 9);
      info_ref = pdf_add_object_drop (ctx, doc, pdf_info);
      pdf_dict_puts_drop (ctx, pdf_trailer (ctx, doc), "Info", info_ref);
    }
  else if (append == false)
    {
      new_info = pdf_new_dict (ctx, doc, 9);
      pdf_update_object (ctx, doc, pdf_to_num (ctx, pdf_info), new_info);
      pdf_drop_obj (ctx, new_info);
      pdf_info = new_info;
    }
  
  if (info == NULL)
    /* Empty info dict.  */
    return;

  int len = pdfout_data_hash_len (ctx, info);
  for (int i = 0; i < len; ++i)
    {
      char *key, *value;
      int value_len;
      pdfout_data_hash_get_key_value (ctx, info, &key, &value, &value_len, i);
      
      if (!strcmp (key, "Trapped"))
	/* create name object */
	pdf_dict_puts_drop (ctx, pdf_info, key,
			      pdf_new_name (ctx, doc, value));
      else
	insert_key_value (ctx, doc, pdf_info, key, value, value_len);
    }
}

pdfout_data *
pdfout_info_dict_get (fz_context *ctx, pdf_document *doc)
{
  /* FIXME: do proper try/catch.  */
  pdf_obj *info = pdf_dict_gets (ctx, pdf_trailer (ctx, doc), "Info");
  int len = pdf_dict_len (ctx, info);

  pdfout_data *result = pdfout_data_hash_new (ctx);
  
  for (int i = 0; i < len; ++i)
    {
      pdf_obj *key = pdf_dict_get_key (ctx, info, i);
      if (pdf_is_name (ctx, key) == false)
	pdfout_throw (ctx, "key in info dict not a name object");

      const char *name = pdf_to_name (ctx, key);
      pdf_obj *val = pdf_dict_get_val (ctx, info, i);
      if (!strcmp (name, "Title") || !strcmp (name, "Author")
	  || !strcmp (name, "Subject")|| !strcmp (name, "Keywords")
	  || !strcmp (name, "Creator") || !strcmp (name, "Producer")
	  || !strcmp (name, "CreationDate") || !strcmp (name, "ModDate"))
	{
	  if (pdf_is_string (ctx, val) == false)
	    pdfout_throw (ctx, "value of info dict key '%s' not a string",
			  name);

	  int string_len;
	  char *string = pdfout_str_obj_to_utf8 (ctx, val, &string_len);
	  pdfout_data_hash_push_key_value (ctx, result, name, string,
					  string_len);
	  free (string);
	}
      else if (strcmp (name, "Trapped") == 0)
	{
	  if (pdf_is_name (ctx, val) == 0)
	    pdfout_throw (ctx, "key '%s' in info dict not a name object", name);
	  
	  char *value_string = pdf_to_name (ctx, val);
	  
	  pdfout_data_hash_push_key_value (ctx, result, name, value_string,
					   strlen (value_string));

	}
    }
  return result;
}

