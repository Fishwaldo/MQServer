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
**  xds_engine_xdr.c: XDR encoding/decoding engine
*/

#include <string.h>
#include "xds.h"

/*
 * Encode/decode signed 32-bit integer values.
 */

int
xdr_encode_int32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint32_t tmp;
	xds_int32_t value;

	xds_init_encoding_engine (4);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint32_t);
	if (value < 0) {
		value = 0 - value;
		tmp = 0 - (xds_uint32_t) value;
	} else
		tmp = (xds_uint32_t) value;
	((xds_uint8_t *) buffer)[0] = (tmp >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (tmp >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (tmp >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (tmp >> 0) & 0x000000ff;

	return XDS_OK;
}

int
xdr_decode_int32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int32_t *value;
	xds_uint32_t tmp;

	xds_init_decoding_engine (4);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_int32_t *);
	xds_check_parameter (value != NULL);

	if (((xds_uint8_t *) buffer)[0] & 0x80) {
		/* negative number */
		tmp = ((xds_uint8_t *) buffer)[0];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[1];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[2];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[3];
		tmp = 0 - tmp;
		*value = 0 - (int32_t) tmp;
	} else {
		/* positive number */
		*value = ((xds_uint8_t *) buffer)[0];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[1];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[2];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[3];
	}

	return XDS_OK;
}

/*
 * Encode/decode unsigned 32-bit integer values.
 */

int
xdr_encode_uint32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint32_t value;

	xds_init_encoding_engine (4);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint32_t);
	((xds_uint8_t *) buffer)[0] = (value >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (value >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (value >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (value >> 0) & 0x000000ff;

	return XDS_OK;
}

int
xdr_decode_uint32 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint32_t *value;

	xds_init_decoding_engine (4);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint32_t *);
	xds_check_parameter (value != NULL);

	*value = ((xds_uint8_t *) buffer)[0];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[1];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[2];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[3];

	return XDS_OK;
}

#ifdef XDS_HAVE_64_BIT_SUPPORT

/*
 * Encode/decode signed 64-bit integer values.
 */

int
xdr_encode_int64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint64_t tmp;
	xds_int64_t value;

	xds_init_encoding_engine (8);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint64_t);
	if (value < 0) {
		value = 0 - value;
		tmp = 0 - (xds_uint64_t) value;
	} else
		tmp = (xds_uint64_t) value;
	((xds_uint8_t *) buffer)[0] = (tmp >> 56) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (tmp >> 48) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (tmp >> 40) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (tmp >> 32) & 0x000000ff;
	((xds_uint8_t *) buffer)[4] = (tmp >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[5] = (tmp >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[6] = (tmp >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[7] = (tmp >> 0) & 0x000000ff;

	return XDS_OK;
}

int
xdr_decode_int64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_int64_t *value;
	xds_uint64_t tmp;

	xds_init_decoding_engine (8);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_int64_t *);
	xds_check_parameter (value != NULL);

	if (((xds_uint8_t *) buffer)[0] & 0x80) {
		/* negative number */
		tmp = ((xds_uint8_t *) buffer)[0];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[1];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[2];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[3];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[4];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[5];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[6];
		tmp = tmp << 8;
		tmp += ((xds_uint8_t *) buffer)[7];
		tmp = 0 - tmp;
		*value = 0 - (xds_int64_t) tmp;
	} else {
		/* positive number */
		*value = ((xds_uint8_t *) buffer)[0];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[1];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[2];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[3];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[4];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[5];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[6];
		*value = *value << 8;
		*value += ((xds_uint8_t *) buffer)[7];
	}

	return XDS_OK;
}

/*
 * Encode/decode unsigned 64-bit integer values.
 */

int
xdr_encode_uint64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint64_t value;

	xds_init_encoding_engine (8);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint64_t);
	((xds_uint8_t *) buffer)[0] = (value >> 56) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (value >> 48) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (value >> 40) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (value >> 32) & 0x000000ff;
	((xds_uint8_t *) buffer)[4] = (value >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[5] = (value >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[6] = (value >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[7] = (value >> 0) & 0x000000ff;

	return XDS_OK;
}

int
xdr_decode_uint64 (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint64_t *value;

	xds_init_decoding_engine (8);

	/* Get value and format it into the buffer. */
	value = va_arg (*args, xds_uint64_t *);
	xds_check_parameter (value != NULL);

	*value = ((xds_uint8_t *) buffer)[0];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[1];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[2];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[3];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[4];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[5];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[6];
	*value = *value << 8;
	*value += ((xds_uint8_t *) buffer)[7];

	return XDS_OK;
}

#endif /* XDS_HAVE_64_BIT_SUPPORT */

/*
 * Encode/decode floating point values.
 */

typedef struct {
	xds_uint8_t sign;	/* :1 */
	xds_uint32_t fraction;	/* :23 */
	xds_int8_t exponent;	/* :8 */
} myfloat_t;

static int
float2myfloat (myfloat_t * new_num, float num)
{
	size_t i;
	float tmp;

	/* Handle zero as a special case. */

	if (num == 0.0) {
		new_num->sign = 0;
		new_num->fraction = 0;
		new_num->exponent = -127;
		return 0;
	}

	/* Determine the sign of the number. */

	if (num < 0.0) {
		new_num->sign = 1;
		num = 0.0 - num;
	} else
		new_num->sign = 0;

	/* Canonify the number before we convert it. */

	new_num->exponent = 0;
	while (num < 1.0) {
		num *= 2.0;
		--new_num->exponent;
	}

	/* Find the exponent. */

	for (i = 0, tmp = 1.0; i <= 128; ++i, tmp *= 2.0) {
		if (tmp * 2.0 > num)
			break;
	}
	if (i <= 128) {
		num = num / tmp - 1.0;
		new_num->exponent += i;
	} else
		return 1;

	/* Calculate the fraction part. */

	for (new_num->fraction = 0, i = 0; i < 23; ++i) {
		new_num->fraction *= 2.0;
		if (num >= 1.0 / 2.0) {
			new_num->fraction += 1.0;
			num = num * 2.0 - 1.0;
		} else
			num *= 2.0;
	}

	return 0;
}


int
xdr_encode_float (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	myfloat_t value;
	xds_uint8_t tmp;

	xds_init_encoding_engine (4);

	/* Get value and format it into the structure. */
	float2myfloat (&value, (float) va_arg (*args, double));

	memset (buffer, 0, 4);

	if (value.sign == 1)
		((xds_uint8_t *) buffer)[0] |= 0x80;

	tmp = value.exponent + 127;
	((xds_uint8_t *) buffer)[0] |= tmp >> 1;
	((xds_uint8_t *) buffer)[1] |= (tmp & 0x01) << 7;

	((xds_uint8_t *) buffer)[1] |= (xds_uint8_t) ((value.fraction & 0x7fffff) >> 16);
	((xds_uint8_t *) buffer)[2] |= (xds_uint8_t) ((value.fraction & 0x00ffff) >> 8);
	((xds_uint8_t *) buffer)[3] |= (xds_uint8_t) ((value.fraction & 0x0000ff) >> 0);

	return XDS_OK;
}

int
xdr_decode_float (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	float *value;
	xds_uint32_t fraction;
	xds_uint8_t exponent;
	size_t i;
	char sign;

	xds_init_decoding_engine (4);

	value = va_arg (*args, float *);
	*value = 0.0;

	sign = (((xds_uint8_t *) buffer)[0] & 0x80) >> 7;
	exponent = (((xds_uint8_t *) buffer)[0] & 0x7f) << 1;
	exponent += (((xds_uint8_t *) buffer)[1] & 0x80) >> 7;
	fraction = (((xds_uint8_t *) buffer)[1] & 0x7fffff) << 16;
	fraction += ((xds_uint8_t *) buffer)[2] << 8;
	fraction += ((xds_uint8_t *) buffer)[3];

	if (fraction == 0 && exponent == 0)
		return XDS_OK;

	for (i = 23; i > 0; --i) {
		if ((fraction & 0x01) == 1)
			*value += 1.0;
		*value /= 2.0;
		fraction /= 2.0;
	}
	*value += 1.0;

	if (exponent > 127) {
		for (exponent -= 127; exponent > 0; --exponent)
			*value *= 2.0;
	} else {
		for (exponent = 127 - exponent; exponent > 0; --exponent)
			*value /= 2.0;
	}

	if (sign == 1)
		*value = 0.0 - *value;

	return XDS_OK;
}


/*
 * Encode/decode double-precision floating point values.
 */

typedef struct {
	xds_uint8_t sign;	/* :1 */
	xds_uint64_t fraction;	/* :52 */
	xds_int16_t exponent;	/* :11 */
} mydouble_t;

static int
double2mydouble (mydouble_t * new_num, double num)
{
	size_t i;
	double tmp;

	/* Handle zero as a special case. */

	if (num == 0.0) {
		new_num->sign = 0;
		new_num->fraction = 0;
		new_num->exponent = -1023;
		return 0;
	}

	/* Determine the sign of the number. */

	if (num < 0.0) {
		new_num->sign = 1;
		num = 0.0 - num;
	} else
		new_num->sign = 0;

	/* Canonify the number before we convert it. */

	new_num->exponent = 0;
	while (num < 1.0) {
		num *= 2.0;
		--new_num->exponent;
	}

	/* Find the exponent. */

	for (i = 0, tmp = 1.0; i <= 1024; ++i, tmp *= 2.0) {
		if (tmp * 2.0 > num)
			break;
	}
	if (i <= 1024) {
		num = num / tmp - 1.0;
		new_num->exponent += i;
	} else
		return 1;

	/* Calculate the fraction part. */

	for (new_num->fraction = 0, i = 0; i < 52; ++i) {
		new_num->fraction *= 2.0;
		if (num >= 1.0 / 2.0) {
			new_num->fraction += 1.0;
			num = num * 2.0 - 1.0;
		} else
			num *= 2.0;
	}

	return 0;
}


int
xdr_encode_double (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	mydouble_t value;
	xds_uint16_t tmp;

	xds_init_encoding_engine (8);

	/* Get value and format it into the structure. */
	double2mydouble (&value, va_arg (*args, double));

	memset (buffer, 0, 8);

	if (value.sign == 1)
		((xds_uint8_t *) buffer)[0] |= 0x80;

	tmp = value.exponent + 1023;
	((xds_uint8_t *) buffer)[0] |= (tmp >> 4) & 0x7f;
	((xds_uint8_t *) buffer)[1] |= (tmp & 0x0f) << 4;

	((xds_uint8_t *) buffer)[1] |= (xds_uint8_t) ((value.fraction & 0x0f000000000000) >> 48);
	((xds_uint8_t *) buffer)[2] |= (xds_uint8_t) ((value.fraction & 0x00ff0000000000) >> 40);
	((xds_uint8_t *) buffer)[3] |= (xds_uint8_t) ((value.fraction & 0x0000ff00000000) >> 32);
	((xds_uint8_t *) buffer)[4] |= (xds_uint8_t) ((value.fraction & 0x000000ff000000) >> 24);
	((xds_uint8_t *) buffer)[5] |= (xds_uint8_t) ((value.fraction & 0x00000000ff0000) >> 16);
	((xds_uint8_t *) buffer)[6] |= (xds_uint8_t) ((value.fraction & 0x0000000000ff00) >> 8);
	((xds_uint8_t *) buffer)[7] |= (xds_uint8_t) ((value.fraction & 0x000000000000ff) >> 0);

	return XDS_OK;
}

int
xdr_decode_double (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	double *value;
	xds_uint64_t fraction;
	xds_uint16_t exponent;
	size_t i;
	char sign;

	xds_init_decoding_engine (8);

	value = va_arg (*args, double *);
	*value = 0.0;

	sign = (((xds_uint8_t *) buffer)[0] & 0x80) >> 7;
	exponent = (((xds_uint8_t *) buffer)[0] & 0x7f) << 4;
	exponent += (((xds_uint8_t *) buffer)[1] & 0xf0) >> 4;

	fraction = (xds_uint64_t) ((((xds_uint8_t *) buffer)[1] & 0x0f)) << 48;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[2]) << 40;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[3]) << 32;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[4]) << 24;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[5]) << 16;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[6]) << 8;
	fraction += (xds_uint64_t) (((xds_uint8_t *) buffer)[7]) << 0;

	if (fraction == 0 && exponent == 0)
		return XDS_OK;

	for (i = 52; i > 0; --i) {
		if ((fraction & 0x01) == 1)
			*value += 1.0;
		*value /= 2.0;
		fraction /= 2.0;
	}
	*value += 1.0;

	if (exponent > 1023) {
		for (exponent -= 1023; exponent > 0; --exponent)
			*value *= 2.0;
	} else {
		for (exponent = 1023 - exponent; exponent > 0; --exponent)
			*value /= 2.0;
	}

	if (sign == 1)
		*value = 0.0 - *value;

	return XDS_OK;
}

/*
 * Encode/decode NUL-terminated character strings.
 */

int
xdr_encode_string (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	char *p;
	size_t p_len;
	size_t padding;

	xds_init_encoding_engine (4);

	/* Get value from stack and calculate the correct amount of padding. */
	p = va_arg (*args, char *);
	xds_check_parameter (p != NULL);
	p_len = strlen (p);
	padding = (4 - (p_len & 0x03)) & 0x03;
	assert ((p_len + padding) % 4 == 0);

	/* We need (4 + p_len + padding) bytes in the buffer to format our
	   parameter. If we don't have them, return an underflow error. */
	*used_buffer_size = 4 + p_len + padding;
	if (buffer_size < *used_buffer_size)
		return XDS_ERR_OVERFLOW;

	/* Format the values into the buffer. */
	((xds_uint8_t *) buffer)[0] = (p_len >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (p_len >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (p_len >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (p_len >> 0) & 0x000000ff;
	memmove ((xds_uint8_t *) buffer + 4, p, p_len);
	memset ((xds_uint8_t *) buffer + 4 + p_len, 0, padding);

	return XDS_OK;
}

int
xdr_decode_string (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	char **p;
	size_t p_len;
	size_t padding;

	xds_init_decoding_engine (4);

	/* Get value. */
	p = va_arg (*args, char **);
	xds_check_parameter (p != NULL);

	/* Read the size of the message. */
	p_len = ((xds_uint8_t *) buffer)[0];
	p_len = p_len << 8;
	p_len += ((xds_uint8_t *) buffer)[1];
	p_len = p_len << 8;
	p_len += ((xds_uint8_t *) buffer)[2];
	p_len = p_len << 8;
	p_len += ((xds_uint8_t *) buffer)[3];

	/* Calculate padding. */
	padding = (4 - (p_len & 0x03)) & 0x03;

	/* Do we have enough data? */
	*used_buffer_size = 4 + p_len + padding;
	if (buffer_size < *used_buffer_size)
		return XDS_ERR_UNDERFLOW;

	/* Allocate buffer for the data. */
	*p = (char *) malloc (p_len + 1);
	if (*p == NULL)
		return XDS_ERR_NO_MEM;

	/* Copy data into the buffer. */
	memmove (*p, (xds_uint8_t *) buffer + 4, p_len+1);
	((xds_uint8_t *) buffer)[4 + p_len] = '\0';
	return XDS_OK;
}

/*
 * Encode/decode octet streams.
 */

int
xdr_encode_octetstream (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	xds_uint8_t *p;
	size_t p_len;
	size_t padding;

	xds_init_encoding_engine (4);

	/* Get value from stack and calculate the correct amount of padding. */
	p = (xds_uint8_t *) va_arg (*args, void *);
	xds_check_parameter (p != NULL);
	p_len = va_arg (*args, size_t);
	padding = (4 - (p_len & 0x03)) & 0x03;
	assert ((p_len + padding) % 4 == 0);

	/* We need (4 + p_len + padding) bytes in the buffer to format our
	   parameter. If we don't have them, return an underflow error. */
	*used_buffer_size = 4 + p_len + padding;
	if (buffer_size < *used_buffer_size)
		return XDS_ERR_OVERFLOW;

	/* Format the values into the buffer. */
	((xds_uint8_t *) buffer)[0] = (p_len >> 24) & 0x000000ff;
	((xds_uint8_t *) buffer)[1] = (p_len >> 16) & 0x000000ff;
	((xds_uint8_t *) buffer)[2] = (p_len >> 8) & 0x000000ff;
	((xds_uint8_t *) buffer)[3] = (p_len >> 0) & 0x000000ff;
	memmove ((xds_uint8_t *) buffer + 4, p, p_len);
	memset ((xds_uint8_t *) buffer + 4 + p_len, 0, padding);

	return XDS_OK;
}

int
xdr_decode_octetstream (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	void **p;
	size_t *p_len;
	size_t padding;

	xds_init_decoding_engine (4);

	p = va_arg (*args, void **);
	p_len = va_arg (*args, size_t *);
	xds_check_parameter (p != NULL);
	xds_check_parameter (p_len != NULL);

	/* Read the size of the message. */
	*p_len = ((xds_uint8_t *) buffer)[0];
	*p_len = *p_len << 8;
	*p_len += ((xds_uint8_t *) buffer)[1];
	*p_len = *p_len << 8;
	*p_len += ((xds_uint8_t *) buffer)[2];
	*p_len = *p_len << 8;
	*p_len += ((xds_uint8_t *) buffer)[3];

	/* Calculate padding. */
	padding = (4 - (*p_len & 0x03)) & 0x03;

	/* Do we have enough data? */
	*used_buffer_size = 4 + *p_len + padding;
	if (buffer_size < *used_buffer_size)
		return XDS_ERR_UNDERFLOW;

	/* Allocate buffer for the data. */
	*p = malloc (*p_len);
	if (*p == NULL)
		return XDS_ERR_NO_MEM;

	/* Copy data into the buffer. */
	memmove (*p, (xds_uint8_t *) buffer + 4, *p_len);

	return XDS_OK;
}
