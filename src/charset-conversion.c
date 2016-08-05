/* The pdfout document modification and analysis tool.
   Copyright (C) 1999-2001, 2008, 2011 Free Software Foundation, Inc.
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

/*
  The mbtowc_xxx and wctomb_xxx functions below are taken from
   the GNU LIBICONV library.
*/

#include "common.h"
#include "charset-conversion.h"
#include "minmax.h"
#include "c-strcase.h"

#ifndef ucs4_t
# define ucs4_t uint32_t
#endif


/* Return code if invalid input after a shift sequence of n bytes was read.
   (xxx_mbtowc) */
#define RET_SHIFT_ILSEQ(n)  (-1-2*(n))
/* Return code if invalid. (xxx_mbtowc) */
#define RET_ILSEQ           RET_SHIFT_ILSEQ(0)
/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW(n)       (-2-2*(n))

/* Return code if invalid. (xxx_wctomb) */
#define RET_ILUNI      -1
/* Return code if output buffer is too small. (xxx_wctomb) */
#define RET_TOOSMALL   -2

typedef struct conv {
  /* Initially all set to 0.  */
  int ostate;
  unsigned istate;
} *conv_t;

typedef unsigned state_t;

/*
 * ASCII
 */

static int
ascii_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
	      _GL_UNUSED int n)
{
  unsigned char c = *s;
  if (c < 0x80) {
    *pwc = (ucs4_t) c;
    return 1;
  }
  return RET_ILSEQ;
}

static int
ascii_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc,
	      _GL_UNUSED int n)
{
  if (wc < 0x0080) {
    *r = wc;
    return 1;
  }
  return RET_ILUNI;
}

/*
 * UTF-8
 */

/* Specification: RFC 3629 */

static int
utf8_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
	     int n)
{
  unsigned char c = s[0];

  if (c < 0x80)
    {
      *pwc = c;
      return 1;
    }
  else if (c < 0xc2)
    return RET_ILSEQ;
  else if (c < 0xe0)
    {
      if (n < 2)
	return RET_TOOFEW(0);
      if (!((s[1] ^ 0x80) < 0x40))
	return RET_ILSEQ;
      *pwc = ((ucs4_t) (c & 0x1f) << 6) | (ucs4_t) (s[1] ^ 0x80);
      return 2;
    }
  else if (c < 0xf0)
    {
      if (n < 3)
	return RET_TOOFEW(0);
      if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
	    && (c >= 0xe1 || s[1] >= 0xa0)))
	return RET_ILSEQ;
      *pwc = ((ucs4_t) (c & 0x0f) << 12) | ((ucs4_t) (s[1] ^ 0x80) << 6)
	| (ucs4_t) (s[2] ^ 0x80);
      return 3;
    }
  else if (c < 0xf8 && sizeof(ucs4_t)*8 >= 32)
    {
      if (n < 4)
	return RET_TOOFEW(0);
      if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
	    && (s[3] ^ 0x80) < 0x40 && (c >= 0xf1 || s[1] >= 0x90)))
	return RET_ILSEQ;
      *pwc = ((ucs4_t) (c & 0x07) << 18) | ((ucs4_t) (s[1] ^ 0x80) << 12)
	| ((ucs4_t) (s[2] ^ 0x80) << 6) | (ucs4_t) (s[3] ^ 0x80);
      return 4;
    }
  else if (c < 0xfc && sizeof(ucs4_t)*8 >= 32)
    {
      if (n < 5)
	return RET_TOOFEW(0);
      if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
	    && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
	    && (c >= 0xf9 || s[1] >= 0x88)))
	return RET_ILSEQ;
      *pwc = ((ucs4_t) (c & 0x03) << 24) | ((ucs4_t) (s[1] ^ 0x80) << 18)
	| ((ucs4_t) (s[2] ^ 0x80) << 12) | ((ucs4_t) (s[3] ^ 0x80) << 6)
	| (ucs4_t) (s[4] ^ 0x80);
      return 5;
    }
  else if (c < 0xfe && sizeof(ucs4_t)*8 >= 32)
    {
      if (n < 6)
	return RET_TOOFEW(0);
      if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
	    && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
	    && (s[5] ^ 0x80) < 0x40 && (c >= 0xfd || s[1] >= 0x84)))
	return RET_ILSEQ;
      *pwc = ((ucs4_t) (c & 0x01) << 30) | ((ucs4_t) (s[1] ^ 0x80) << 24)
	| ((ucs4_t) (s[2] ^ 0x80) << 18) | ((ucs4_t) (s[3] ^ 0x80) << 12)
	| ((ucs4_t) (s[4] ^ 0x80) << 6) | (ucs4_t) (s[5] ^ 0x80);
      return 6;
    }
  else
    return RET_ILSEQ;
}

static int
utf8_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  int count;
  if (wc < 0x80)
    count = 1;
  else if (wc < 0x800)
    count = 2;
  else if (wc < 0x10000)
    count = 3;
  else if (wc < 0x200000)
    count = 4;
  else if (wc < 0x4000000)
    count = 5;
  else if (wc <= 0x7fffffff)
    count = 6;
  else
    return RET_ILUNI;
  if (n < count)
    return RET_TOOSMALL;
  switch (count)
    { /* note: code falls through cases! */
    case 6: r[5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
    case 5: r[4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
    case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
    case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
    case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
    case 1: r[0] = wc;
      break;
    default:
      abort();
    }
  return count;
}

/*
 * UTF-16BE
 */

/* Specification: RFC 2781 */

static int
utf16be_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
		int n)
{
  int count = 0;
  if (n >= 2)
    {
      ucs4_t wc = (s[0] << 8) + s[1];
      if (wc >= 0xd800 && wc < 0xdc00)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc2 = (s[2] << 8) + s[3];
	      if (!(wc2 >= 0xdc00 && wc2 < 0xe000))
		goto ilseq;
	      *pwc = 0x10000 + ((wc - 0xd800) << 10) + (wc2 - 0xdc00);
	      return count+4;
	    }
	}
      else if (wc >= 0xdc00 && wc < 0xe000)
	  goto ilseq;
      else
	{
	  *pwc = wc;
	  return count+2;
	}
    }
  return RET_TOOFEW(count);

 ilseq:
  return RET_ILSEQ;
}

static int
utf16be_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (!(wc >= 0xd800 && wc < 0xe000))
    {
      if (wc < 0x10000)
	{
	  if (n >= 2)
	    {
	      r[0] = (unsigned char) (wc >> 8);
	      r[1] = (unsigned char) wc;
	      return 2;
	    }
	  else
	    return RET_TOOSMALL;
	}
      else if (wc < 0x110000)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc1 = 0xd800 + ((wc - 0x10000) >> 10);
	      ucs4_t wc2 = 0xdc00 + ((wc - 0x10000) & 0x3ff);
	      r[0] = (unsigned char) (wc1 >> 8);
	      r[1] = (unsigned char) wc1;
	      r[2] = (unsigned char) (wc2 >> 8);
	      r[3] = (unsigned char) wc2;
	      return 4;
	    }
	  else
	    return RET_TOOSMALL;
	}
    }
  return RET_ILUNI;
}


/*
 * UTF-16LE
 */

/* Specification: RFC 2781 */

static int
utf16le_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
		int n)
{
  int count = 0;
  if (n >= 2)
    {
      ucs4_t wc = s[0] + (s[1] << 8);
      if (wc >= 0xd800 && wc < 0xdc00)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc2 = s[2] + (s[3] << 8);
	      if (!(wc2 >= 0xdc00 && wc2 < 0xe000))
		goto ilseq;
	      *pwc = 0x10000 + ((wc - 0xd800) << 10) + (wc2 - 0xdc00);
	      return count+4;
	    }
	}
      else if (wc >= 0xdc00 && wc < 0xe000)
	goto ilseq;
      else
	{
	  *pwc = wc;
	  return count+2;
	}
    }
  return RET_TOOFEW(count);

 ilseq:
  return RET_ILSEQ;
}

static int
utf16le_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc,
		int n)
{
  if (!(wc >= 0xd800 && wc < 0xe000))
    {
      if (wc < 0x10000)
	{
	  if (n >= 2)
	    {
	      r[0] = (unsigned char) wc;
	      r[1] = (unsigned char) (wc >> 8);
	      return 2;
	    }
	  else
	    return RET_TOOSMALL;
	}
      else if (wc < 0x110000)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc1 = 0xd800 + ((wc - 0x10000) >> 10);
	      ucs4_t wc2 = 0xdc00 + ((wc - 0x10000) & 0x3ff);
	      r[0] = (unsigned char) wc1;
	      r[1] = (unsigned char) (wc1 >> 8);
	      r[2] = (unsigned char) wc2;
	      r[3] = (unsigned char) (wc2 >> 8);
	      return 4;
	    }
	  else
	    return RET_TOOSMALL;
	}
    }
  return RET_ILUNI;
}

/*
 * UTF-16
 */

/* Specification: RFC 2781 */

/* Here we accept FFFE/FEFF marks as endianness indicators everywhere
   in the stream, not just at the beginning. (This is contrary to what
   RFC 2781 section 3.2 specifies, but it allows concatenation of byte
   sequences to work flawlessly, while disagreeing with the RFC behaviour
   only for strings containing U+FEFF characters, which is quite rare.)
   The default is big-endian. */
/* The state is 0 if big-endian, 1 if little-endian. */
static int
utf16_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  state_t state = conv->istate;
  int count = 0;
  for (; n >= 2;)
    {
      ucs4_t wc = (state ? s[0] + (s[1] << 8) : (s[0] << 8) + s[1]);
      if (wc == 0xfeff)
	{}
      else if (wc == 0xfffe)
	state ^= 1;
      else if (wc >= 0xd800 && wc < 0xdc00)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc2 = (state ? s[2] + (s[3] << 8) : (s[2] << 8) + s[3]);
	      if (!(wc2 >= 0xdc00 && wc2 < 0xe000))
		goto ilseq;
	      *pwc = 0x10000 + ((wc - 0xd800) << 10) + (wc2 - 0xdc00);
	      conv->istate = state;
	      return count+4;
	    }
	  else
	    break;
	}
      else if (wc >= 0xdc00 && wc < 0xe000)
	goto ilseq;
      else
	{
	  *pwc = wc;
	  conv->istate = state;
	  return count+2;
	}
      s += 2; n -= 2; count += 2;
    }
  conv->istate = state;
  return RET_TOOFEW(count);

 ilseq:
  conv->istate = state;
  return RET_SHIFT_ILSEQ(count);
}

/* We output UTF-16 in big-endian order, with byte-order mark.
   See RFC 2781 section 3.3 for a rationale: Some document formats
   mandate a BOM; the file concatenation issue is not so severe as
   long as the above utf16_mbtowc function is used. */
/* The state is 0 at the beginning, 1 after the BOM has been written. */
static int
utf16_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc != 0xfffe && !(wc >= 0xd800 && wc < 0xe000))
    {
      int count = 0;
      if (!conv->ostate)
	{
	  if (n >= 2)
	    {
	      r[0] = 0xFE;
	      r[1] = 0xFF;
	      r += 2; n -= 2; count += 2;
	    }
	  else
	    return RET_TOOSMALL;
	}
      if (wc < 0x10000)
	{
	  if (n >= 2)
	    {
	      r[0] = (unsigned char) (wc >> 8);
	      r[1] = (unsigned char) wc;
	      conv->ostate = 1;
	      return count+2;
	    }
	  else
	    return RET_TOOSMALL;
	}
      else if (wc < 0x110000)
	{
	  if (n >= 4)
	    {
	      ucs4_t wc1 = 0xd800 + ((wc - 0x10000) >> 10);
	      ucs4_t wc2 = 0xdc00 + ((wc - 0x10000) & 0x3ff);
	      r[0] = (unsigned char) (wc1 >> 8);
	      r[1] = (unsigned char) wc1;
	      r[2] = (unsigned char) (wc2 >> 8);
	      r[3] = (unsigned char) wc2;
	      conv->ostate = 1;
	      return count+4;
	    }
	  else
	    return RET_TOOSMALL;
	}
    }
  return RET_ILUNI;
}


/*
 * UTF-32BE
 */

/* Specification: Unicode 3.1 Standard Annex #19 */

static int
utf32be_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
		int n)
{
  if (n >= 4)
    {
      ucs4_t wc = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
      if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
	{
	  *pwc = wc;
	  return 4;
	}
      else
	return RET_ILSEQ;
    }
  return RET_TOOFEW(0);
}

static int
utf32be_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
    {
      if (n >= 4)
	{
	  r[0] = 0;
	  r[1] = (unsigned char) (wc >> 16);
	  r[2] = (unsigned char) (wc >> 8);
	  r[3] = (unsigned char) wc;
	  return 4;
	}
      else
	return RET_TOOSMALL;
    }
  return RET_ILUNI;
}

/*
 * UTF-32LE
 */

/* Specification: Unicode 3.1 Standard Annex #19 */

static int
utf32le_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
		int n)
{
  if (n >= 4)
    {
      ucs4_t wc = s[0] + (s[1] << 8) + (s[2] << 16) + (s[3] << 24);
      if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
	{
	  *pwc = wc;
	  return 4;
	}
      else
	return RET_ILSEQ;
    }
  return RET_TOOFEW(0);
}

static int
utf32le_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
    {
      if (n >= 4)
	{
	  r[0] = (unsigned char) wc;
	  r[1] = (unsigned char) (wc >> 8);
	  r[2] = (unsigned char) (wc >> 16);
	  r[3] = 0;
	  return 4;
	}
      else
	return RET_TOOSMALL;
    }
  return RET_ILUNI;
}

/*
 * UTF-32
 */

/* Specification: Unicode 3.1 Standard Annex #19 */

/* Here we accept FFFE0000/0000FEFF marks as endianness indicators
   everywhere in the stream, not just at the beginning. (This is contrary
   to what #19 D36c specifies, but it allows concatenation of byte
   sequences to work flawlessly, while disagreeing with #19 behaviour
   only for strings containing U+FEFF characters, which is quite rare.)
   The default is big-endian. */
/* The state is 0 if big-endian, 1 if little-endian. */
static int
utf32_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  state_t state = conv->istate;
  int count = 0;
  for (; n >= 4;)
    {
      ucs4_t wc = (state
		   ? s[0] + (s[1] << 8) + (s[2] << 16) + (s[3] << 24)
		   : (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3]);
      if (wc == 0x0000feff)
	{}
      else if (wc == 0xfffe0000u)
	state ^= 1;
      else
	{
	  if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
	    {
	      *pwc = wc;
	      conv->istate = state;
	      return count+4;
	    }
	  else
	    {
	      conv->istate = state;
	      return RET_SHIFT_ILSEQ(count);
	    }
	}
      s += 4; n -= 4; count += 4;
    }
  conv->istate = state;
  return RET_TOOFEW(count);
}

/* We output UTF-32 in big-endian order, with byte-order mark. */
/* The state is 0 at the beginning, 1 after the BOM has been written. */
static int
utf32_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x110000 && !(wc >= 0xd800 && wc < 0xe000))
    {
      int count = 0;
      if (!conv->ostate)
	{
	  if (n >= 4)
	    {
	      r[0] = 0x00;
	      r[1] = 0x00;
	      r[2] = 0xFE;
	      r[3] = 0xFF;
	      r += 4; n -= 4; count += 4;
	    }
	  else
	    return RET_TOOSMALL;
	}
      if (wc < 0x110000)
	{
	  if (n >= 4)
	    {
	      r[0] = 0;
	      r[1] = (unsigned char) (wc >> 16);
	      r[2] = (unsigned char) (wc >> 8);
	      r[3] = (unsigned char) wc;
	      conv->ostate = 1;
	      return count+4;
	    }
	  else
	    return RET_TOOSMALL;
	}
    }
  return RET_ILUNI;
}

/* The following tables, mbtowc and wctomb functions were generated from
   PDFDOCENCODING.TXT using GNU libiconv's 8bit_tab_to_h program:

   ./8bit_tab_to_h PDFDOCENCODING pdfdoc < PDFDOCENCODING.TXT
*/

/*
 * PDFDOCENCODING
 */

static const unsigned short pdfdoc_2uni_1[16] = {
  /* 0x10 */
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
  0x02d8, 0x02c7, 0x02c6, 0x02d9, 0x02dd, 0x02db, 0x02da, 0x02dc,
};
static const unsigned short pdfdoc_2uni_2[64] = {
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

static int
pdfdoc_mbtowc (_GL_UNUSED conv_t conv, ucs4_t *pwc, const unsigned char *s,
	       _GL_UNUSED int n)
{
  unsigned char c = *s;
  if (c < 0x10)
    {
      *pwc = (ucs4_t) c;
      return 1;
    }
  else if (c < 0x20)
    {
      *pwc = (ucs4_t) pdfdoc_2uni_1[c-0x10];
      return 1;
    }
  else if (c < 0x70)
    {
      *pwc = (ucs4_t) c;
      return 1;
    }
  else if (c < 0xb0)
    {
      unsigned short wc = pdfdoc_2uni_2[c-0x70];
      if (wc != 0xfffd)
	{
	  *pwc = (ucs4_t) wc;
	  return 1;
	}
    }
  else
    {
      *pwc = (ucs4_t) c;
      return 1;
    }
  return RET_ILSEQ;
}

static const unsigned char pdfdoc_page00[56] = {
  0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x00, /* 0x78-0x7f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x98-0x9f */
  0x00, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, /* 0xa0-0xa7 */
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0x00, 0xae, 0xaf, /* 0xa8-0xaf */
};
static const unsigned char pdfdoc_page01[104] = {
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
static const unsigned char pdfdoc_page02[32] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1a, 0x19, /* 0xc0-0xc7 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xc8-0xcf */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0xd0-0xd7 */
  0x18, 0x1b, 0x1e, 0x1d, 0x1f, 0x1c, 0x00, 0x00, /* 0xd8-0xdf */
};
static const unsigned char pdfdoc_page20[56] = {
  0x00, 0x00, 0x00, 0x85, 0x84, 0x00, 0x00, 0x00, /* 0x10-0x17 */
  0x8f, 0x90, 0x91, 0x00, 0x8d, 0x8e, 0x8c, 0x00, /* 0x18-0x1f */
  0x81, 0x82, 0x80, 0x00, 0x00, 0x00, 0x83, 0x00, /* 0x20-0x27 */
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28-0x2f */
  0x8b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
  0x00, 0x88, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
  0x00, 0x00, 0x00, 0x00, 0x87, 0x00, 0x00, 0x00, /* 0x40-0x47 */
};
static const unsigned char pdfdoc_pagefb[8] = {
  0x00, 0x93, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x00-0x07 */
};

static int
pdfdoc_wctomb (_GL_UNUSED conv_t conv, unsigned char *r, ucs4_t wc,
	       _GL_UNUSED int n)
{
  unsigned char c = 0;
  if (wc < 0x0018)
    {
      *r = wc;
      return 1;
    }
  else if (wc >= 0x0020 && wc < 0x0078)
    c = wc;
  else if (wc >= 0x0078 && wc < 0x00b0)
    c = pdfdoc_page00[wc-0x0078];
  else if (wc >= 0x00b0 && wc < 0x0100)
    c = wc;
  else if (wc >= 0x0130 && wc < 0x0198)
    c = pdfdoc_page01[wc-0x0130];
  else if (wc >= 0x02c0 && wc < 0x02e0)
    c = pdfdoc_page02[wc-0x02c0];
  else if (wc >= 0x2010 && wc < 0x2048)
    c = pdfdoc_page20[wc-0x2010];
  else if (wc == 0x20ac)
    c = 0xa0;
  else if (wc == 0x2122)
    c = 0x92;
  else if (wc == 0x2212)
    c = 0x8a;
  else if (wc >= 0xfb00 && wc < 0xfb08)
    c = pdfdoc_pagefb[wc-0xfb00];
  if (c != 0)
    {
      *r = c;
      return 1;
    }
  return RET_ILUNI;
}


typedef int (*mbtowc_func) (conv_t conv, ucs4_t *pwc, const unsigned char *s,
			    int n);
typedef int (*wctomb_func) (conv_t conv, unsigned char *r, ucs4_t wc, int n);
/* List of supported encodings.  */
struct encoding
{
  const char *name;
  mbtowc_func mbtowc;
  wctomb_func wctomb;
};

#include "encodings.h"

char *
pdfout_char_conv (fz_context *ctx, const char *fromcode, const char *tocode,
		  const char *src, int srclen, int *lengthp)
{
  fz_buffer *buf = fz_new_buffer (ctx, 1);
  unsigned char *result;
  
  fz_try (ctx)
  {
    pdfout_char_conv_buffer (ctx, fromcode, tocode, src, srclen, buf);
  
    /* Zero-terminate.  */
    fz_write_buffer (ctx, buf, "\0\0\0\0", 4);
  
  
    *lengthp = fz_buffer_storage (ctx, buf, &result) - 4;
  }
  fz_catch (ctx)
  {
    fz_drop_buffer (ctx, buf);
    fz_rethrow (ctx);
  }

  /* Hack: Just free the buffer's struct, but not it's data.  */
  free (buf);
  return (char *) result;
}

void
pdfout_char_conv_buffer (fz_context *ctx, const char *fromcode,
			 const char *tocode, const char *src, int srclen,
			 fz_buffer *buf)
{
  mbtowc_func mbtowc = NULL;
  wctomb_func wctomb = NULL;
    
  struct encoding *encoding = get_encoding (fromcode, strlen (fromcode));
  
  if (encoding == NULL)
    pdfout_throw (ctx, "unknown encoding '%s'", fromcode);
  
  mbtowc = encoding->mbtowc;
  
  encoding = get_encoding (tocode, strlen (tocode));
  
  if (encoding == NULL)
    pdfout_throw (ctx, "unknown encoding '%s'", tocode);
  
  wctomb = encoding->wctomb;

  struct conv conv = {0};

  while (srclen)
    {
      ucs4_t pwc;
      int read;
	
      read = mbtowc (&conv, &pwc, (const unsigned char *) src,
		     MIN (10, srclen));
      if (read < 0)
	pdfout_throw (ctx, "pdfout_charset_conv: invalid %s multibyte",
		      fromcode);
      src += read;
      srclen -= read;

      unsigned char tmp[10];
      int written = wctomb (&conv, tmp, pwc, 10);
	
      if (written > 0)
	fz_write_buffer (ctx, buf, tmp, written);
      else if (written == RET_ILUNI)
	fz_throw (ctx, FZ_ERROR_ABORT,
		  "pdfout_charset_conv: codepoint 0x%x invalid in %s",
		  pwc, tocode);
      else
	abort();
    }
}
    

char *
pdfout_pdf_to_utf8 (fz_context *ctx, const char *inbuf, int inbuf_len,
		    int *outbuf_len)
{
  if (inbuf_len >= 2
      && (memcmp (inbuf, "\xfe\xff", 2) == 0
	  || memcmp (inbuf, "\xff\xfe", 2) == 0))
    return pdfout_char_conv (ctx, "UTF-16", "C", inbuf, inbuf_len,
			     outbuf_len);
  else
    return pdfout_char_conv (ctx, "PDFDOC", "C", inbuf, inbuf_len,
			     outbuf_len);
}

char *
pdfout_utf8_to_pdf (fz_context *ctx, const char *inbuf, int inbuf_len,
		    int *outbuf_len)
{
  /* First try pdfdoc encoding. If that fails, use UTF-16BE.  */

  char *result;
  bool use_pdfdoc = false;
  
  fz_var (result);
  fz_var (use_pdfdoc);
  
  fz_try (ctx)
  {
    result = pdfout_char_conv (ctx, "UTF-8", "PDFDOC", inbuf, inbuf_len,
			       outbuf_len);
    use_pdfdoc = true;
  }
  fz_catch (ctx)
  {
    fz_rethrow_if (ctx, FZ_ERROR_GENERIC);
  }

  if (use_pdfdoc)
    return result;

  return pdfout_char_conv (ctx, "UTF-8", "UTF-16", inbuf, inbuf_len,
			   outbuf_len);
}

pdf_obj *
pdfout_utf8_to_str_obj (fz_context *ctx, pdf_document *doc,
		       const char *inbuf, int inbuf_len)
{
  int pdf_text_len;
  char *pdf_text = pdfout_utf8_to_pdf (ctx, inbuf, inbuf_len, &pdf_text_len);

  /* FIXME: proper try/catch */
  pdf_obj *result =  pdf_new_string (ctx, doc, pdf_text, pdf_text_len);

  free (pdf_text);

  return result;
}

char *
pdfout_str_obj_to_utf8 (fz_context *ctx, pdf_obj *string, int *len)
{
  const char *text = pdf_to_str_buf (ctx, string);
  int text_len = pdf_to_str_len (ctx, string);

  /* FIXME: proper try/catch */
  return pdfout_pdf_to_utf8 (ctx, text, text_len, len);
}
