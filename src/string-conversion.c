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

#include <unistr.h>

enum pdfout_text_string_mode
pdfout_text_string_mode = PDFOUT_TEXT_STRING_DEFAULT;

static char *pdfdoc_to_utf8 (enum iconv_ilseq_handler handler, const char *src,
			     size_t srclen, size_t *lengthp);
  
char *
pdfout_text_string_to_utf8 (char *inbuf, int inbuf_len, int *outbuf_len)
{
  assert (inbuf);
  char *utf8;
  size_t utf8_len;
  char *from;

  if (inbuf_len >= 2
      && (memcmp (inbuf, "\xfe\xff", 2) == 0
	  || memcmp (inbuf, "\xff\xfe", 2) == 0))
    {
      from = "UTF-16";
      utf8 = (char *) u8_conv_from_encoding (from, iconveh_question_mark,
					     inbuf, inbuf_len, NULL, NULL,
					     &utf8_len);
    }
  else
    {
      from = "PDFDOC";
      utf8 = pdfdoc_to_utf8 (iconveh_question_mark, inbuf,
			     inbuf_len, &utf8_len);
    }

   
  if (utf8 == NULL)
    error (1, errno, "conversion from %s to UTF-8 failed", from);
  if (utf8_len > INT_MAX)
    error (1, 0, "overflow in pdfout_text_string_to_utf8");

  utf8 = xrealloc (utf8, utf8_len + 1);
  utf8[utf8_len] = '\0';

  *outbuf_len = utf8_len;
  return utf8;
}

static char *utf8_to_pdfdoc (enum iconv_ilseq_handler handler, char *inbuf,
			     size_t inbuf_len, size_t *outbuf_len);
static char *utf8_to_utf16be (enum iconv_ilseq_handler handler, char *inbuf,
			      size_t inbuf_len, size_t *outbuf_len);
char *
pdfout_utf8_to_text_string (char *inbuf, int inbuf_len, int *outbuf_len)
{
  char *result;
  size_t result_len;
  enum pdfout_text_string_mode mode = pdfout_text_string_mode;
  
  if (mode != PDFOUT_TEXT_STRING_UTF16)
    {
      enum iconv_ilseq_handler handler = iconveh_error;
      
      if (mode == PDFOUT_TEXT_STRING_PDFDOC_QUESTION_MARK)
	handler = iconveh_question_mark;
      
      result = utf8_to_pdfdoc (handler, inbuf, inbuf_len, &result_len);
  
      if (result)
	goto check_overflow;
      else if (mode == PDFOUT_TEXT_STRING_PDFDOC_ERROR)
	{
	  pdfout_msg ("string '%s' cannot be converted to PDFDocEncoding",
		      inbuf);
	  exit (1);
	}
    }
  
  /* The error handler should not matter, since libyaml and our wysiwyg parser
     already caught invalid UTF-8  */
  
  result = utf8_to_utf16be (iconveh_error, inbuf, inbuf_len, &result_len);

  if (result == NULL)
    error (1, errno, "conversion from UTF8 to UTF-16BE failed for string '%s'",
	   inbuf);
  
 check_overflow:
  if (result_len > INT_MAX)
    error (1, 0, "overflow in pdfout_utf8_to_text_string");

  *outbuf_len = result_len;
  
  return result;
}

static char *
utf8_to_utf16be (enum iconv_ilseq_handler handler, char *inbuf,
		 size_t inbuf_len, size_t *outbuf_len)
{
  size_t utf8_len = 3 + inbuf_len;
  uint8_t *utf8 = xmalloc (utf8_len);
  char *outbuf;
  assert (inbuf);

  /* UTF-8 BOM, will get translated into "\xfe\xff" */
  utf8[0] = 0xef;
  utf8[1] = 0xbb;
  utf8[2] = 0xbf;
  
  memcpy (&utf8[3], inbuf, inbuf_len);
  
  outbuf = u8_conv_to_encoding ("UTF-16BE", handler, utf8,
				utf8_len, NULL, NULL, outbuf_len);
  if (outbuf == NULL)
    error (1, errno, "conversion from UTF-8 to UTF-16BE failed for string:"
	   " '%s'", inbuf);
  
  free (utf8);
  return outbuf;
}

static char *utf32_to_pdfdoc (enum iconv_ilseq_handler handler,
			      const uint32_t *src, size_t srclen,
			      size_t *lengthp);
static char *
utf8_to_pdfdoc (enum iconv_ilseq_handler handler, char *inbuf,
		size_t inbuf_len, size_t *outbuf_len)
{
  char *outbuf;
  uint32_t *utf32;
  size_t utf32_len;
  
  assert (inbuf);

  utf32 = u8_to_u32 ((uint8_t *) inbuf, inbuf_len, NULL, &utf32_len);
  if (utf32 == NULL)
    error (1, errno, "pdfout_utf8_to_pdfdoc: u8_to_u32 failed");
  
  outbuf = utf32_to_pdfdoc (handler, utf32, utf32_len, outbuf_len);
  
  free (utf32);
  
  return outbuf;
}

#define MSG(format, args...)					\
  pdfout_msg ("pdfdoc to utf8 conversion: " format, ## args)

static uint32_t *pdfdoc_to_utf32 (enum iconv_ilseq_handler handler,
				  const char *src, size_t srclen,
				  size_t *lengthp);

/* returns NULL or error */
static char *
pdfdoc_to_utf8 (enum iconv_ilseq_handler handler, const char *src,
		size_t srclen, size_t *lengthp)
{
  uint32_t *utf32;
  size_t utf32len;
  char *utf8;
  
  utf32 = pdfdoc_to_utf32 (handler, src, srclen, &utf32len);
  if (utf32 == NULL)
    return NULL;
  
  utf8 = (char *) u32_to_u8 (utf32, utf32len, NULL, lengthp);

  free (utf32);
  
  return utf8;
}

static int pdfdocencoding_mbtowc (ucs4_t *pwc, const unsigned char *s);
static int pdfdocencoding_wctomb (unsigned char *r, ucs4_t wc);

/*returns 0-terminated UTF-32 string.
  On error, returns NULL */
static uint32_t *
pdfdoc_to_utf32 (enum iconv_ilseq_handler handler, const char *src,
		 size_t srclen, size_t *lengthp)

{
  uint32_t *utf32;
  size_t i = 0;
  
  assert (handler != iconveh_escape_sequence);
  
 *lengthp = srclen;
 
 utf32 = XNMALLOC (srclen + 1, uint32_t);

 for (i = 0; i < srclen; ++i)
   {
     if (pdfdocencoding_mbtowc (&utf32[i], (unsigned char *) &src[i]) != 1)
       {
	 if (handler == iconveh_error)
	   {
	     free (utf32);
	     return NULL;
	   }
	 utf32[i] = '?';
       }
   }

 /* terminating 0 */
 utf32[i] = 0;
 
 return utf32;
}

static char *
utf32_to_pdfdoc (enum iconv_ilseq_handler handler, const uint32_t *src,
		 size_t srclen, size_t *lengthp)
{
  unsigned char *pdfdoc;
  size_t i = 0;
  
  assert (handler != iconveh_escape_sequence);
  
  *lengthp = srclen;

  pdfdoc = xmalloc (*lengthp);

  for (i = 0; i < *lengthp; ++i)
    {
      if (pdfdocencoding_wctomb (&pdfdoc[i], src[i]) != 1)
	{
	  if (handler == iconveh_error)
	    {
	      free (pdfdoc);
	      return NULL;
	    }
	  pdfdoc[i] = '?';
	}
    }
  
  return (char *) pdfdoc;
}

/* the following tables and functions were generated from PDFDOCENCODING.TXT
   using GNU libiconv's 8bit_tab_to_h program */
  
/*
 * PDFDocEncoding
 */

static const unsigned short pdfdocencoding_2uni_1[16] = {
  /* 0x10 */
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0017, 0x0017,
  0x02d8, 0x02c7, 0x02c6, 0x02d9, 0x02dd, 0x02db, 0x02da, 0x02dc,
};
static const unsigned short pdfdocencoding_2uni_2[64] = {
  /* 0x70 */
  0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
  0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0xfffd,
  /* 0x80 */
  0x2022, 0x2020, 0x2021, 0x2026, 0x2014, 0x2013, 0x0192, 0x2044,
  0x2039, 0x203a, 0x2212, 0x2030, 0x201e, 0x201c, 0x201d, 0x2018,
  /* 0x90 */
  0x2019, 0x201a, 0x2122, 0xfb01, 0xfb02, 0x0141, 0x0152, 0x0160,
  0x0178, 0x017d, 0x0131, 0x0142, 0x0153, 0x0161, 0x017e, 0xfffd,
  /* 0xa0 */
  0x20ac, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
  0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0xfffd, 0x00ae, 0x00af,
};

#define RET_ILSEQ -1

static int
pdfdocencoding_mbtowc (ucs4_t *pwc, const unsigned char *s)
{
  unsigned char c = *s;
  if (c < 0x10) {
    *pwc = (ucs4_t) c;
    return 1;
  }
  else if (c < 0x20) {
    *pwc = (ucs4_t) pdfdocencoding_2uni_1[c-0x10];
    return 1;
  }
  else if (c < 0x70) {
    *pwc = (ucs4_t) c;
    return 1;
  }
  else if (c < 0xb0) {
    unsigned short wc = pdfdocencoding_2uni_2[c-0x70];
    if (wc != 0xfffd) {
      *pwc = (ucs4_t) wc;
      return 1;
    }
  }
  else {
    *pwc = (ucs4_t) c;
    return 1;
  }
  return RET_ILSEQ;
}

static const unsigned char pdfdocencoding_page00[8] = {
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x00, 0x17, /* 0x10-0x17 */
};
static const unsigned char pdfdocencoding_page00_1[56] = {
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x00, /* 0x78-0x7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98-0x9f */
  0x00, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 0xa0-0xa7 */
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0x00, 0xae, 0xaf, /* 0xa8-0xaf */
};
static const unsigned char pdfdocencoding_page01[104] = {
  0x00, 0x9a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
  0x00, 0x95, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40-0x47 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x48-0x4f */
  0x00, 0x00, 0x96, 0x9c, 0x00, 0x00, 0x00, 0x00, /* 0x50-0x57 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x58-0x5f */
  0x97, 0x9d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60-0x67 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x68-0x6f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70-0x77 */
  0x98, 0x00, 0x00, 0x00, 0x00, 0x99, 0x9e, 0x00, /* 0x78-0x7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
  0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
};
static const unsigned char pdfdocencoding_page02[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x19, /* 0xc0-0xc7 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc8-0xcf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0-0xd7 */
  0x18, 0x1b, 0x1e, 0x1d, 0x1f, 0x1c, 0x00, 0x00, /* 0xd8-0xdf */
};
static const unsigned char pdfdocencoding_page20[56] = {
  0x00, 0x00, 0x00, 0x85, 0x84, 0x00, 0x00, 0x00, /* 0x10-0x17 */
  0x8f, 0x90, 0x91, 0x00, 0x8d, 0x8e, 0x8c, 0x00, /* 0x18-0x1f */
  0x81, 0x82, 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, /* 0x20-0x27 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28-0x2f */
  0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
  0x00, 0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
  0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, /* 0x40-0x47 */
};
static const unsigned char pdfdocencoding_pagefb[8] = {
  0x00, 0x93, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00-0x07 */
};

#define RET_ILUNI -1

static int
pdfdocencoding_wctomb (unsigned char *r, ucs4_t wc)
{
  unsigned char c = 0;
  if (wc < 0x0010) {
    *r = wc;
    return 1;
  }
  else if (wc >= 0x0010 && wc < 0x0018)
    c = pdfdocencoding_page00[wc-0x0010];
  else if (wc >= 0x0020 && wc < 0x0078)
    c = wc;
  else if (wc >= 0x0078 && wc < 0x00b0)
    c = pdfdocencoding_page00_1[wc-0x0078];
  else if (wc >= 0x00b0 && wc < 0x0100)
    c = wc;
  else if (wc >= 0x0130 && wc < 0x0198)
    c = pdfdocencoding_page01[wc-0x0130];
  else if (wc >= 0x02c0 && wc < 0x02e0)
    c = pdfdocencoding_page02[wc-0x02c0];
  else if (wc >= 0x2010 && wc < 0x2048)
    c = pdfdocencoding_page20[wc-0x2010];
  else if (wc == 0x20ac)
    c = 0xa0;
  else if (wc == 0x2122)
    c = 0x92;
  else if (wc == 0x2212)
    c = 0x8a;
  else if (wc >= 0xfb00 && wc < 0xfb08)
    c = pdfdocencoding_pagefb[wc-0xfb00];
  if (c != 0) {
    *r = c;
    return 1;
  }
  return RET_ILUNI;
}
