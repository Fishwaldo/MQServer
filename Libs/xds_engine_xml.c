/*
**  OSSP xds - Extensible Data Serialization
**  Copyright (c) 2001-2003 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2001-2003 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2001-2003 Cable & Wireless Germany <http://www.cw.com/de/>
**
**  This file is part of OSSP xds, an extensible data serialization
**  library which can be found at http://www.ossp.org/pkg/lib/xds/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  xds_engine_xml.c: XML encoding/decoding engine
*/

#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "xds.h"

/*
 * Encode/decode XML document framework
 */

static const char xds_xml_begin_text[] = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\">\n" "<!DOCTYPE xds SYSTEM \"http://www.ossp.org/pkg/lib/xds/xds-xml.dtd\">\n" "<xds>\n";

static const char xds_xml_end_text[] = "</xds>\n";

int
xml_encode_begin (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_encoding_engine (strlen (xds_xml_begin_text));
	memmove (buffer, xds_xml_begin_text, strlen (xds_xml_begin_text));
	return XDS_OK;
}

int
xml_decode_begin (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_encoding_engine (strlen (xds_xml_begin_text));
	if (strncasecmp (buffer, xds_xml_begin_text, strlen (xds_xml_begin_text)) != 0)
		return XDS_ERR_TYPE_MISMATCH;
	return XDS_OK;
}

int
xml_encode_end (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_encoding_engine (strlen (xds_xml_end_text));
	memmove (buffer, xds_xml_end_text, strlen (xds_xml_end_text));
	return XDS_OK;
}

int
xml_decode_end (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_decoding_engine (strlen (xds_xml_end_text));
	if (strncasecmp (buffer, xds_xml_end_text, strlen (xds_xml_end_text)) != 0)
		return XDS_ERR_TYPE_MISMATCH;
	return XDS_OK;
}

/*
 * Encode/decode signed 32-bit integer values.
 */

int
xml_encode_int32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int32_t value;
	char buf[32];
	size_t i, j;
	char *p;
	int negative;

	xds_init_encoding_engine (7 + 8 + 11);

	/* Get the value and format it into our buffer. Keep track of the length
	   of the formatted result. */
	value = va_arg (*args, xds_int32_t);
	if (value < 0) {
		negative = XDS_TRUE;
		value = 0 - value;
	} else
		negative = XDS_FALSE;
	i = 0;
	do {
		unsigned char remainder = value % 10;
		value = value / 10;
		buf[i++] = '0' + remainder;
	} while (value != 0);
	if (negative)
		buf[i++] = '-';

	/* Now that we know the correct size of our data's representation, write
	   it into the buffer. */
	*used_buffer_size = 7 + 8 + i;
	p = buffer;
	memmove (p, "<int32>", 7);
	p += 7;
	for (j = i; j > 0;)
		*p++ = buf[--j];
	memmove (p, "</int32>", 8);

	return XDS_OK;
}

int
xml_decode_int32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int32_t *value;
	char *p;
	int negative;

	xds_init_decoding_engine (7 + 8 + 1);

	/* Does the opening XML tag match? */
	if (strncasecmp (buffer, "<int32>", 7) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Decode the representation of the value. */
	value = va_arg (*args, xds_int32_t *);
	*value = 0;
	p = (char *) buffer + 7;
	if (*p == '-') {
		negative = XDS_TRUE;
		p++;
	} else
		negative = XDS_FALSE;
	while (isdigit ((int) *p)) {
		if (p >= (char *) buffer + buffer_size)
			return XDS_ERR_UNDERFLOW;
		*value *= 10;
		*value += *p++ - '0';
	}
	if (negative)
		*value = 0 - *value;

	/* Does the closing XML tag match? */
	if (p + 8 > (char *) buffer + buffer_size)
		return XDS_ERR_UNDERFLOW;
	else if (strncasecmp (p, "</int32>", 8) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	*used_buffer_size = p + 8 - (char *) buffer;
	return XDS_OK;
}

/*
 * Encode/decode unsigned 32-bit integer values.
 */

int
xml_encode_uint32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint32_t value;
	char buf[32];
	size_t i, j;
	char *p;

	xds_init_encoding_engine (8 + 9 + 10);

	/* Format value into our buffer. */
	value = va_arg (*args, xds_uint32_t);
	i = 0;
	do {
		unsigned char remainder = value % 10;
		value = value / 10;
		buf[i++] = '0' + remainder;
	} while (value != 0);

	/* Store the correct buffer size. */
	*used_buffer_size = 8 + 9 + i;

	/* Write result into the buffer. */
	p = buffer;
	memmove (p, "<uint32>", 8);
	p += 8;
	for (j = i; j > 0;)
		*p++ = buf[--j];
	memmove (p, "</uint32>", 9);

	return XDS_OK;
}

int
xml_decode_uint32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint32_t *value;
	char *p;

	xds_init_decoding_engine (8 + 9 + 1);

	/* Does the opening XML tag match? */
	if (strncasecmp (buffer, "<uint32>", 8) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Decode the representation of the value. */
	value = va_arg (*args, xds_uint32_t *);
	*value = 0;
	p = (char *) buffer + 8;
	while (isdigit ((int) *p)) {
		if (p >= (char *) buffer + buffer_size)
			return XDS_ERR_UNDERFLOW;
		*value *= 10;
		*value += *p++ - '0';
	}

	/* Does the closing XML tag match? */
	if (p + 9 > (char *) buffer + buffer_size)
		return XDS_ERR_UNDERFLOW;
	else if (strncasecmp (p, "</uint32>", 9) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	*used_buffer_size = p + 9 - (char *) buffer;
	return XDS_OK;
}

#ifdef XDS_HAVE_64_BIT_SUPPORT

/*
 * Encode/decode signed 64-bit integer values.
 */

int
xml_encode_int64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int64_t value;
	char buf[64];
	size_t i, j;
	char *p;
	int negative;

	xds_init_encoding_engine (7 + 8 + 21);

	/* Format value into our buffer. */
	value = va_arg (*args, xds_int64_t);
	if (value < 0) {
		negative = XDS_TRUE;
		value = 0 - value;
	} else
		negative = XDS_FALSE;
	i = 0;
	do {
		unsigned char remainder = value % 10;
		value = value / 10;
		buf[i++] = '0' + remainder;
	} while (value != 0);
	if (negative)
		buf[i++] = '-';

	/* Store the correct buffer size. */
	*used_buffer_size = 7 + 8 + i;

	/* Write result into the buffer. */
	p = buffer;
	memmove (p, "<int64>", 7);
	p += 7;
	for (j = i; j > 0;)
		*p++ = buf[--j];
	memmove (p, "</int64>", 8);

	return XDS_OK;
}

int
xml_decode_int64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int64_t *value;
	char *p;
	int negative;

	xds_init_decoding_engine (7 + 8 + 1);

	/* Does the opening XML tag match? */
	if (strncasecmp (buffer, "<int64>", 7) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Decode the representation of the value. */
	value = va_arg (*args, xds_int64_t *);
	*value = 0;
	p = (char *) buffer + 7;
	if (*p == '-') {
		negative = XDS_TRUE;
		p++;
	} else
		negative = XDS_FALSE;
	while (isdigit ((int) *p)) {
		if (p >= (char *) buffer + buffer_size)
			return XDS_ERR_UNDERFLOW;
		*value *= 10;
		*value += *p++ - '0';
	}
	if (negative)
		*value = 0 - *value;

	/* Does the closing XML tag match? */
	if (p + 8 > (char *) buffer + buffer_size)
		return XDS_ERR_UNDERFLOW;
	else if (strncasecmp (p, "</int64>", 8) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	*used_buffer_size = p + 8 - (char *) buffer;
	return XDS_OK;
}

/*
 * Encode/decode unsigned 64-bit integer values.
 */

int
xml_encode_uint64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint64_t value;
	char buf[64];
	size_t i, j;
	char *p;

	xds_init_encoding_engine (8 + 9 + 20);

	/* Format value into our buffer. */
	value = va_arg (*args, xds_uint64_t);
	i = 0;
	do {
		unsigned char remainder = value % 10;
		value = value / 10;
		buf[i++] = '0' + remainder;
	} while (value != 0);

	/* Store the correct buffer size. */
	*used_buffer_size = 8 + 9 + i;

	/* Write result into the buffer. */
	p = buffer;
	memmove (p, "<uint64>", 8);
	p += 8;
	for (j = i; j > 0;)
		*p++ = buf[--j];
	memmove (p, "</uint64>", 9);

	return XDS_OK;
}

int
xml_decode_uint64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint64_t *value;
	char *p;

	xds_init_decoding_engine (8 + 9 + 1);

	/* Does the opening XML tag match? */
	if (strncasecmp (buffer, "<uint64>", 8) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Decode the representation of the value. */
	value = va_arg (*args, xds_uint64_t *);
	*value = 0;
	p = (char *) buffer + 8;
	while (isdigit ((int) *p)) {
		if (p >= (char *) buffer + buffer_size)
			return XDS_ERR_UNDERFLOW;
		*value *= 10;
		*value += *p++ - '0';
	}

	/* Does the closing XML tag match? */
	if (p + 9 > (char *) buffer + buffer_size)
		return XDS_ERR_UNDERFLOW;
	else if (strncasecmp (p, "</uint64>", 9) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	*used_buffer_size = p + 9 - (char *) buffer;
	return XDS_OK;
}

#endif /* XDS_HAVE_64_BIT_SUPPORT */

/*
 * Encode/decode floating point values.
 */

int
xml_encode_float (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_encoding_engine (7 + 8 + 1);
	*used_buffer_size = snprintf (buffer, buffer_size, "<float>%f</float>", va_arg (*args, double));
	if (*used_buffer_size >= buffer_size)
		return XDS_ERR_OVERFLOW;
	else
		return XDS_OK;
}

int
xml_decode_float (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_decoding_engine (7 + 8 + 1);
	if (sscanf (buffer, "<float>%f</float>%n", va_arg (*args, float *), used_buffer_size) != 1)
		  return XDS_ERR_TYPE_MISMATCH;
	else
		return XDS_OK;
}

/*
 * Encode/decode double-precision floating point values.
 */

int
xml_encode_double (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_encoding_engine (8 + 9 + 1);
	*used_buffer_size = snprintf (buffer, buffer_size, "<double>%f</double>", va_arg (*args, double));
	if (*used_buffer_size >= buffer_size)
		return XDS_ERR_OVERFLOW;
	else
		return XDS_OK;
}

int
xml_decode_double (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_init_decoding_engine (8 + 9 + 1);
	if (sscanf (buffer, "<double>%lf</double>%n", va_arg (*args, double *), used_buffer_size) != 1)
		  return XDS_ERR_TYPE_MISMATCH;
	else
		return XDS_OK;
}

/*
 * Encode/decode NUL-terminated character strings.
 */

#define bits(c)     (0x80 | ((c) & 0x3F))
#define put(c)      *strptr++ = (c);
#define putbits(c)  put(bits(c))
#define finish()    *strptr = '\0'

static char *
sputu8 (xds_uint32_t c, char *strptr)
{
	if (strptr == NULL)
		return NULL;

	if (c < 0x80) {
		put (c);
		finish ();
	} else if (c < 0x800) {
		put (0xC0 | (c >> 6));
		putbits (c);
		finish ();
	} else if (c < 0x10000) {
		put (0xE0 | (c >> 12));
		putbits (c >> 6);
		putbits (c);
		finish ();
	} else if (c < 0x200000) {
		put (0xF0 | (c >> 18));
		putbits (c >> 12);
		putbits (c >> 6);
		putbits (c);
		finish ();
	} else if (c < 0x400000) {
		put (0xF8 | (c >> 24));
		putbits (c >> 18);
		putbits (c >> 12);
		putbits (c >> 6);
		putbits (c);
		finish ();
	} else if (c < 0x80000000) {
		put (0xFC | (c >> 30));
		putbits (c >> 24);
		putbits (c >> 18);
		putbits (c >> 12);
		putbits (c >> 6);
		putbits (c);
		finish ();
	} else
		finish ();	/* Not a valid Unicode "character" */

	return strptr;
}

static const char TAG_OPEN[] = "<string>";
static const char TAG_CLOSE[] = "</string>";
static const size_t TAG_OPEN_LEN = sizeof (TAG_OPEN) - 1;
static const size_t TAG_CLOSE_LEN = sizeof (TAG_CLOSE) - 1;

int
xml_encode_string (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	char *src;
	size_t src_len;
	char *dst;
	size_t dst_size;
	char *tmp;

	/* Setup the engine. We need at least space for our tags; how long the
	   actual content is going to be will be seen soon. */
	xds_init_encoding_engine (TAG_OPEN_LEN + TAG_CLOSE_LEN);

	/* Get the data from the stack. */
	src = va_arg (*args, char *);
	xds_check_parameter (src != NULL);
	src_len = strlen (src);

	/* Set up the target buffer. */
	dst = buffer;
	dst_size = buffer_size;

	/* Write the opening tag. */
	memmove (dst, TAG_OPEN, TAG_OPEN_LEN);
	dst += TAG_OPEN_LEN;
	dst_size -= TAG_OPEN_LEN;

	/* Format the data into the buffer. */
	while (src_len > 0 && dst_size > 0) {
		if (*((xds_uint8_t *) src) >= 0x80) {
			/* UTF-8ify it. */
			if (dst_size >= 7) {
				tmp = sputu8 ((xds_uint32_t) * ((xds_uint8_t *) src), dst);
				src++;
				src_len--;
				dst_size -= tmp - dst;
				dst = tmp;
			} else
				dst_size = 0;
		} else if (*src == '<') {
			/* Turn into "&lt;". */
			if (dst_size >= 4) {
				*dst++ = '&';
				dst_size--;
				*dst++ = 'l';
				dst_size--;
				*dst++ = 't';
				dst_size--;
				*dst++ = ';';
				dst_size--;
				src++;
				src_len--;
			} else
				dst_size = 0;
		} else if (*src == '&') {
			/* Turn into "&amp;". */
			if (dst_size >= 5) {
				*dst++ = '&';
				dst_size--;
				*dst++ = 'a';
				dst_size--;
				*dst++ = 'm';
				dst_size--;
				*dst++ = 'p';
				dst_size--;
				*dst++ = ';';
				dst_size--;
				src++;
				src_len--;
			} else
				dst_size = 0;
		} else if (*src == '>') {
			/* Turn into "&gt;". */
			if (dst_size >= 4) {
				*dst++ = '&';
				dst_size--;
				*dst++ = 'g';
				dst_size--;
				*dst++ = 't';
				dst_size--;
				*dst++ = ';';
				dst_size--;
				src++;
				src_len--;
			} else
				dst_size = 0;
		} else {
			/* No special character; just copy it. */
			*dst++ = *src++;
			src_len--;
			dst_size--;
		}
	}
	if (src_len > 0) {
		/* Target buffer was too small. */
		*used_buffer_size = buffer_size + 1;
		return XDS_ERR_OVERFLOW;
	}

	/* Write the closing tag. */
	memmove (dst, TAG_CLOSE, TAG_CLOSE_LEN);
	dst += TAG_CLOSE_LEN;
	dst_size -= TAG_CLOSE_LEN;

	*used_buffer_size = dst - (char *) buffer;
	return XDS_OK;
}

#define INVALID \
    0x80000000
#define get(c) \
    c = *strptr++; \
    if (chars) \
        (*chars)++; \
    if ((c) == 0) \
        return (unsigned int)EOF

static unsigned int
sgetu8 (unsigned char *strptr, int *chars)
{
	unsigned int c;
	int i, iterations;
	unsigned char ch;

	if (chars)
		*chars = 0;

	if (strptr == NULL)
		return (unsigned int) EOF;

	get (c);
	if ((c & 0xFE) == 0xFC) {
		c &= 0x01;
		iterations = 5;
	} else if ((c & 0xFC) == 0xF8) {
		c &= 0x03;
		iterations = 4;
	} else if ((c & 0xF8) == 0xF0) {
		c &= 0x07;
		iterations = 3;
	} else if ((c & 0xF0) == 0xE0) {
		c &= 0x0F;
		iterations = 2;
	} else if ((c & 0xE0) == 0xC0) {
		c &= 0x1F;
		iterations = 1;
	} else if ((c & 0x80) == 0x80)
		return INVALID;
	else
		return c;

	for (i = 0; i < iterations; i++) {
		get (ch);
		if ((ch & 0xC0) != 0x80)
			return INVALID;
		c <<= 6;
		c |= ch & 0x3F;
	}

	return c;
}

int
xml_decode_string (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	char **target_buffer;
	char *src;
	size_t src_len;
	char *dst;
	int utf8_len;
	unsigned int rc;

	/* Setup the engine. We need at least space for our tags; how long the
	   actual content is going to be will be seen soon. */
	xds_init_encoding_engine (TAG_OPEN_LEN + TAG_CLOSE_LEN);

	/* Is the opening tag there? */
	if (strncasecmp (buffer, TAG_OPEN, TAG_OPEN_LEN) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Determine the length of the encoded data. */
	src = (char *) buffer + TAG_OPEN_LEN;
	for (src_len = 0; src[src_len] != '<'; ++src_len)
		if (src[src_len] == '\0')
			return XDS_ERR_TYPE_MISMATCH;

	/* Check the closing tag. */
	if (strncasecmp (src + src_len, TAG_CLOSE, TAG_CLOSE_LEN) != 0)
		return XDS_ERR_TYPE_MISMATCH;
	*used_buffer_size = TAG_OPEN_LEN + src_len + TAG_CLOSE_LEN;

	/* Allocate target buffer. */
	target_buffer = va_arg (*args, char **);
	xds_check_parameter (target_buffer != NULL);
	*target_buffer = dst = malloc (src_len + 1);
	if (dst == NULL)
		return XDS_ERR_NO_MEM;

	/* Decode the data into the target buffer. */
	while (src_len > 0) {
		if (*src == '&') {
			if (src_len >= 4 && strncmp (src, "&lt;", 4) == 0) {
				*dst++ = '<';
				src += 4;
				src_len -= 4;
			} else if (src_len >= 4 && strncmp (src, "&gt;", 4) == 0) {
				*dst++ = '>';
				src += 4;
				src_len -= 4;
			} else if (src_len >= 5 && strncmp (src, "&amp;", 5) == 0) {
				*dst++ = '&';
				src += 5;
				src_len -= 5;
			} else {
				free (dst);
				return XDS_ERR_TYPE_MISMATCH;
			}
		} else if (*((xds_uint8_t *) src) >= 0x80) {
			rc = sgetu8 ((xds_uint8_t *) src, &utf8_len);
			if (rc == (unsigned int) EOF)
				return XDS_ERR_UNDERFLOW;
			else if (rc == INVALID || rc > 255)
				return XDS_ERR_TYPE_MISMATCH;
			*dst++ = (xds_uint8_t) rc;
			src += utf8_len;
			src_len -= utf8_len;
		} else {
			*dst++ = *src++;
			src_len--;
		}
	}
	*dst = '\0';

	return XDS_OK;
}

/*
 * Encode/decode octet streams.
 */

static const char xds_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char xds_pad64 = '=';

static int
base64_encode (char *dst, size_t dstlen, unsigned char const *src, size_t srclen)
{
	size_t dstpos;
	unsigned char input[3];
	unsigned char output[4];
	int ocnt;
	size_t i;

	if (srclen == 0)
		return 0;
	if (dst == NULL) {
		/* just calculate required length of dst */
		dstlen = (((srclen + 2) / 3) * 4);
		return dstlen;
	}

	/* bulk encoding */
	dstpos = 0;
	ocnt = 0;
	while (srclen >= 3) {
		input[0] = *src++;
		input[1] = *src++;
		input[2] = *src++;
		srclen -= 3;

		output[0] = (input[0] >> 2);
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);
		output[3] = (input[2] & 0x3f);

		if (dstpos + 4 > dstlen)
			return -1;
		dst[dstpos++] = xds_base64[output[0]];
		dst[dstpos++] = xds_base64[output[1]];
		dst[dstpos++] = xds_base64[output[2]];
		dst[dstpos++] = xds_base64[output[3]];
	}

	/* now worry about padding with remaining 1 or 2 bytes */
	if (srclen != 0) {
		input[0] = input[1] = input[2] = '\0';
		for (i = 0; i < srclen; i++)
			input[i] = *src++;

		output[0] = (input[0] >> 2);
		output[1] = ((input[0] & 0x03) << 4) + (input[1] >> 4);
		output[2] = ((input[1] & 0x0f) << 2) + (input[2] >> 6);

		if (dstpos + 4 > dstlen)
			return -1;
		dst[dstpos++] = xds_base64[output[0]];
		dst[dstpos++] = xds_base64[output[1]];
		if (srclen == 1)
			dst[dstpos++] = xds_pad64;
		else
			dst[dstpos++] = xds_base64[output[2]];
		dst[dstpos++] = xds_pad64;
	}

	if (dstpos >= dstlen)
		return -1;
	dst[dstpos] = '\0';

	return dstpos;
}

int
xml_encode_octetstream (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint8_t *src;
	size_t src_len;

	/* We need at least 27 byte for the starting and ending tag. */
	xds_init_encoding_engine (13 + 14);

	/* Get parameters from stack. */
	src = (xds_uint8_t *) va_arg (*args, void *);
	xds_check_parameter (src != NULL);
	src_len = va_arg (*args, size_t);

	/* Calculate how many bytes we'll need in buffer and make sure we have
	   them. */
	*used_buffer_size = base64_encode (NULL, 0, src, src_len);
	if (*used_buffer_size == (size_t) - 1)
		return XDS_ERR_UNKNOWN;
	else
		*used_buffer_size += 13 + 14;
	if (buffer_size < *used_buffer_size)
		return XDS_ERR_OVERFLOW;

	/* Format the data into the buffer. */
	memmove (buffer, "<octetstream>", 13);
	if (base64_encode ((char *) buffer + 13, buffer_size - 13, src, src_len) < 0)
		return XDS_ERR_UNKNOWN;
	memmove ((char *) buffer + *used_buffer_size - 14, "</octetstream>", 14);

	/* Done. */

	return XDS_OK;
}

static int
base64_decode (unsigned char *dst, size_t dstlen, char const *src, size_t srclen)
{
	int dstidx, state, ch = 0;
	unsigned char res;
	char *pos;

	if (srclen == 0)
		return 0;
	state = 0;
	dstidx = 0;
	res = 0;
	while (srclen-- > 0) {
		ch = *src++;
		if (isascii (ch) && isspace (ch))	/* Skip whitespace anywhere */
			continue;
		if (ch == xds_pad64)
			break;
		pos = strchr (xds_base64, ch);
		if (pos == 0)	/* A non-base64 character */
			return -1;
		switch (state) {
		case 0:
			if (dst != NULL) {
				if ((size_t) dstidx >= dstlen)
					return -1;
				dst[dstidx] = ((pos - xds_base64) << 2);
			}
			state = 1;
			break;
		case 1:
			if (dst != NULL) {
				if ((size_t) dstidx >= dstlen)
					return -1;
				dst[dstidx] |= ((pos - xds_base64) >> 4);
				res = (((pos - xds_base64) & 0x0f) << 4);
			}
			dstidx++;
			state = 2;
			break;
		case 2:
			if (dst != NULL) {
				if ((size_t) dstidx >= dstlen)
					return -1;
				dst[dstidx] = res | ((pos - xds_base64) >> 2);
				res = ((pos - xds_base64) & 0x03) << 6;
			}
			dstidx++;
			state = 3;
			break;
		case 3:
			if (dst != NULL) {
				if ((size_t) dstidx >= dstlen)
					return -1;
				dst[dstidx] = res | (pos - xds_base64);
			}
			dstidx++;
			state = 0;
			break;
		default:
			break;
		}
	}

	/*
	 * We are done decoding Base-64 chars.  Let's see if we ended
	 * on a byte boundary, and/or with erroneous trailing characters.
	 */
	if (ch == xds_pad64) {	/* We got a pad char. */
		switch (state) {
		case 0:	/* Invalid = in first position */
		case 1:	/* Invalid = in second position */
			return -1;
		case 2:	/* Valid, means one byte of info */
			/* Skip any number of spaces. */
			while (srclen > 0) {
				ch = *src++;
				--srclen;
				if (!(isascii (ch) && isspace (ch)))
					break;
			}
			/* Make sure there is another trailing = sign. */
			if (ch != xds_pad64)
				return -1;
			/* FALLTHROUGH */
		case 3:	/* Valid, means two bytes of info */
			/*
			 * We know this char is an =.  Is there anything but
			 * whitespace after it?
			 */
			while (srclen > 0) {
				ch = *src++;
				--srclen;
				if (!(isascii (ch) && isspace (ch)))
					return -1;
			}
			/*
			 * Now make sure for cases 2 and 3 that the "extra"
			 * bits that slopped past the last full byte were
			 * zeros.  If we don't check them, they become a
			 * subliminal channel.
			 */
			if (dst != NULL && res != 0)
				return -1;
		default:
			break;
		}
	} else {
		/*
		 * We ended by seeing the end of the string.  Make sure we
		 * have no partial bytes lying around.
		 */
		if (state != 0)
			return -1;
	}

	return dstidx;
}

int
xml_decode_octetstream (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	char *p;
	size_t p_len;
	xds_uint8_t **data;
	size_t *data_len;

	/* We need at least 27 byte for the starting and ending tag. */
	xds_init_encoding_engine (13 + 14);

	/* Get parameters from stack. */
	data = va_arg (*args, xds_uint8_t **);
	xds_check_parameter (data != NULL);
	data_len = va_arg (*args, size_t *);

	/* Check for the opening tag. */
	if (memcmp ("<octetstream>", buffer, 13) != 0)
		return XDS_ERR_TYPE_MISMATCH;

	/* Find the end of the data and calculate the length of the
	   base64-encoded stuff. */
	p = (char *) buffer + 13;
	while (p < ((char *) buffer + buffer_size) && *p != '<')
		++p;
	if (p == ((char *) buffer + buffer_size))
		return XDS_ERR_TYPE_MISMATCH;
	else {
		p_len = p - ((char *) buffer + 13);
		p = (char *) buffer + 13;
	}

	/* Now find out how long the decoded data is going to be, allocate a
	   buffer for it, and decode away. */
	*data_len = base64_decode (NULL, 0, p, p_len);
	if (*data_len == (size_t) - 1)
		return XDS_ERR_UNKNOWN;
	*data = malloc (*data_len);
	if (*data == NULL)
		return XDS_ERR_NO_MEM;
	base64_decode (*data, *data_len, p, p_len);

	/* Check that we have a closing tag. */
	if (memcmp (p + p_len, "</octetstream>", 14) != 0) {
		free (*data);
		return XDS_ERR_TYPE_MISMATCH;
	}

	*used_buffer_size = 13 + p_len + 14;
	return XDS_OK;
}
