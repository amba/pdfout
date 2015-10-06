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
#include "page-labels.h"
#include <c-ctype.h>

struct pdfout_page_labels_s {
  /* See section 12.4.2 in PDF 1.7 reference.  */
  size_t len, cap;
  pdfout_page_labels_mapping_t *array;
};
  
pdfout_page_labels_t *
pdfout_page_labels_new (void)
{
  return XZALLOC (pdfout_page_labels_t);
}

void
pdfout_page_labels_free (pdfout_page_labels_t *labels)
{
  size_t i;
    
  if (labels == NULL)
    return;
  
  for (i = 0; i < labels->len; ++i)
    free (labels->array[i].prefix);
  
  free (labels->array);
  free (labels);
}

#define MSG(fmt, args...) pdfout_msg ("pdfout_page_labels_push: " fmt, ## args);
#define MSG_RETURN(value, fmt, args...)		\
  do						\
    {						\
      MSG (fmt, ## args);			\
      return value;				\
    }						\
  while (0)

int pdfout_page_labels_push (pdfout_page_labels_t *labels,
			     const pdfout_page_labels_mapping_t *mapping)
{
  int previous_page_number;
  pdfout_page_labels_mapping_t *new_mapping;
  assert (labels);
  
  if (mapping->page < 0)
    MSG_RETURN (1, "page < 1");
  if (labels->len == 0 && mapping->page != 0)
    MSG_RETURN (1, "page for first page label must be 1");
  if (labels->len)
    {
      previous_page_number = labels->array[labels->len - 1].page;
      if (mapping->page <= previous_page_number)
	MSG_RETURN (1, "page number %d not larger than previous page %d",
		    mapping->page + 1, previous_page_number + 1);
    }
  if (mapping->start < 0)
    MSG_RETURN (1, "value of key 'first' must be >= 1");
  
  assert (labels->len <= labels->cap);
  if (labels->len == labels->cap)
    labels->array = PDFOUT_X2NREALLOC (labels->array, &labels->cap,
				       pdfout_page_labels_mapping_t);
  new_mapping = &labels->array[labels->len++];
  new_mapping->page = mapping->page;
  new_mapping->style = mapping->style;
  new_mapping->prefix = mapping->prefix ? xstrdup (mapping->prefix) : NULL;
  new_mapping->start = mapping->start;
  return 0;
}

size_t
pdfout_page_labels_length (const pdfout_page_labels_t *labels)
{
  assert (labels);
  return labels->len;
}

pdfout_page_labels_mapping_t *
pdfout_page_labels_get_mapping (const pdfout_page_labels_t *labels,
				size_t index)
{
  assert (labels && labels->len > index);
  return &labels->array[index];
}

#undef MSG
#define MSG(fmt, args...) pdfout_msg ("checking page labels: " fmt, ## args);

int
pdfout_page_labels_check (const pdfout_page_labels_t *labels, int page_count)
{
  int i, len, page, previous_page = -1;
  const pdfout_page_labels_mapping_t *mapping;

  if (labels == NULL)
    return 0;
  len = pdfout_page_labels_length (labels);
  if (len < 1)
    return 1;
  for (i = 0; i < len; ++i)
    {
      mapping = pdfout_page_labels_get_mapping (labels, i);
      page = mapping->page;
      if (page >= page_count)
	{
	  MSG ("page %d exceedes page count %d", page + 1, page_count);
	  return 1;
	}
      /* Some paranoid sanity checks.  */
      if (page < 0 || page <= previous_page || (i == 0 && page != 0)
	  || mapping->start < 0 || mapping->style < 0  || mapping->style > 5)
	return 1;
      previous_page = page;
    }
  return 0;
}

static char *
arabic_numbering (int value, char *resultbuf, size_t prefix_len,
		  size_t *lengthp)
{
  assert (value >= 1);
  int len = 0;
  for (int copy = value; copy; copy /= 10)
    ++len;
  if (resultbuf == NULL || prefix_len + len + 1 > *lengthp)
    {
      *lengthp = prefix_len + len + 1;
      resultbuf = xcharalloc (*lengthp);
    }
  pdfout_snprintf (resultbuf + prefix_len, *lengthp - prefix_len, "%d", value);
  return resultbuf;
}
  
static char *
letter_numbering (int value, bool uppercase, char *resultbuf,
		  size_t prefix_len, size_t *lengthp)
{
  /* numbering: A to Z, then AA to ZZ ... */
  int len = (--value) / 26 + 1;
  char *p;
  
  if (resultbuf == NULL || prefix_len + len + 1 > *lengthp)
    {
      *lengthp = prefix_len + len + 1;
      resultbuf = xcharalloc (*lengthp);
    }
  for (p = resultbuf + prefix_len; p < resultbuf + prefix_len + len; ++p)
    *p = value % 26 + (uppercase ? 'A' : 'a');
  *p = 0;
  return resultbuf;
}

static const char *roman_hundreds[10] =
  {0, "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm"};
static const char *roman_tens[10] =
  {0, "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc"};
static const char *roman_ones[10] =
  {0, "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix"};
static const int roman_str_lengths[10] =
  {0,  1,   2,    3,     2,    1,   2,    3,     4,      2  };

static char *
get_roman_block (char *p, int value, int power, const char **block,
		 bool uppercase)
{
  int i;
  int num = (value % (power * 10)) / power;
  for (i = 0; i < roman_str_lengths[num]; ++i)
    {
      *p = block[num][i];
      if (uppercase)
	*p = c_toupper (*p);
      ++p;
    }
    
  return p;
}

static int
roman_length (int value)
{
  int len;
  /* Leading Ms. */
  len = value / 1000;
  
  len += roman_str_lengths[(value % 1000) / 100];
  len += roman_str_lengths[(value % 100) / 10];
  len += roman_str_lengths[value % 10];
  return len;
}

static char *
roman_numbering (int value, bool uppercase, char *resultbuf, size_t prefix_len,
		 size_t *lengthp)
{
  char *p;
  int num, len = roman_length (value);
  assert (value > 0);
  
  if (resultbuf == NULL || prefix_len + len + 1 > *lengthp)
    {
      *lengthp = prefix_len + len + 1;
      resultbuf = xcharalloc (*lengthp);
    }
  
  /* Get leading Ms.  */
  num = value / 1000;
  for (p = resultbuf + prefix_len; p < resultbuf + prefix_len + num; ++p)
    *p = uppercase ? 'M' : 'm';
  
  p = get_roman_block (p, value, 100, roman_hundreds, uppercase);
  p = get_roman_block (p, value,  10,     roman_tens, uppercase);
  p = get_roman_block (p, value,   1,     roman_ones, uppercase);
  
  *p = 0;
  return resultbuf;
}  

static int _GL_ATTRIBUTE_PURE
do_search (int page_index, const pdfout_page_labels_t *labels)
{
  int len = labels->len;
  
  assert (len >= 1);
  if (page_index >= labels->array[len - 1].page)
    return len - 1;
  else
    {
      /* Use binary search, not linear search, as there are crazy PDFs where
	 labels->len equals the page count.  */
      int upper = len;
      int lower = 0;
      while (1)
	{
	  int mid, page;
	  if (lower > upper)
	    return upper;
	  mid = (lower + upper) / 2;
	  page = labels->array[mid].page;
	  if (page_index == page)
	    return mid;
	  else if (page_index > page)
	    lower = mid + 1;
	  else if (page_index < page)
	    upper = mid - 1;
	}
    }
}

char *
pdfout_page_labels_print (int page_index, const pdfout_page_labels_t *labels,
			  char *resultbuf, size_t *lengthp)
{
  const pdfout_page_labels_mapping_t *mapping;
  int value, start;
  size_t prefix_len;
  
  assert (page_index >= 0);
  assert (labels);
  assert (labels->len);
  
  mapping = &labels->array[do_search (page_index, labels)];
  value = page_index - mapping->page;
  assert (value >= 0);
  start = mapping->start;
  start += !start;
  value += start;
  
  prefix_len = mapping->prefix ? strlen (mapping->prefix) : 0;
  if (mapping->style)
    {
      bool uppercase = true;
      switch (mapping->style)
	{
	case PDFOUT_PAGE_LABELS_ARABIC:
	  resultbuf = arabic_numbering (value, resultbuf, prefix_len, lengthp);
	  break;
	case PDFOUT_PAGE_LABELS_LOWER:
	  uppercase = false;
	  /* fallthrough */
	case PDFOUT_PAGE_LABELS_UPPER:
	  resultbuf = letter_numbering (value, uppercase, resultbuf,
					prefix_len, lengthp);
	  break;
	case PDFOUT_PAGE_LABELS_LOWER_ROMAN:
	  uppercase = false;
	  /* fallthrough */
	case PDFOUT_PAGE_LABELS_UPPER_ROMAN:
	  resultbuf = roman_numbering (value, uppercase, resultbuf,
				       prefix_len, lengthp);
	  break;
	default:
	  abort ();
	}
    }
  else
    {
      if (resultbuf == NULL || prefix_len + 1 > *lengthp)
	{
	  *lengthp = prefix_len + 1;
	  resultbuf = xcharalloc (*lengthp);
	}
    }
  
  if (prefix_len)
    memcpy (resultbuf, mapping->prefix, prefix_len);
  if (mapping->style == 0)
    resultbuf[prefix_len] = 0;
  
  return resultbuf;
}
