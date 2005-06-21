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
**  xds.c: XDS library framework
*/

#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "xds_p.h"
#include "packet.h"

int
xds_init (xds_t ** xds, xds_mode_t mode)
{
	xds_t *ctx;

	/* Sanity check parameter. */
	assert (xds != NULL);
	if (xds == NULL)
		return XDS_ERR_INVALID_ARG;
	assert (mode == XDS_ENCODE || mode == XDS_DECODE);
	if (mode != XDS_ENCODE && mode != XDS_DECODE)
		return XDS_ERR_INVALID_ARG;

	/* Allocate context structure. */
	if ((ctx = (mqlib_malloc) (sizeof (struct xds_context))) == NULL)
		return XDS_ERR_SYSTEM;	/* errno set by malloc(3) */

	/* Set mode of operation in context. */
	ctx->mode = mode;

	/* Initialize buffer handling. */
	ctx->buffer = NULL;
	ctx->buffer_len = 0;
	ctx->buffer_capacity = 0;
	ctx->we_own_buffer = XDS_FALSE;

	/* Initialize engines map. */
	ctx->engines = NULL;
	ctx->engines_len = 0;
	ctx->engines_capacity = 0;

	*xds = ctx;

	return XDS_OK;
}

int
xds_destroy (xds_t * xds)
{
	size_t i;

	/* Sanity check parameter. */
	assert (xds != NULL);
	if (xds == NULL)
		return XDS_ERR_INVALID_ARG;

	/* Free allocated memory. */
	assert (xds->buffer != NULL || (xds->buffer_capacity == 0 && xds->buffer_len == 0));
	if (xds->buffer != NULL && xds->we_own_buffer)
		(mqlib_free) (xds->buffer);
	assert (xds->engines != NULL || xds->engines_capacity == 0);
	if (xds->engines != NULL) {
		for (i = 0; i < xds->engines_len; i++) {
			assert (xds->engines[i].name != NULL);
			(mqlib_free) (xds->engines[i].name);
		}
		(mqlib_free) (xds->engines);
	}
	(mqlib_free) (xds);

	return XDS_OK;
}

int
xds_setbuffer (xds_t * xds, xds_scope_t flag, void *buffer, size_t buffer_len)
{
	/* Sanity check parameters. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (flag == XDS_GIFT || flag == XDS_LOAN);
	xds_check_parameter ((buffer != NULL && buffer_len != 0)
			     || flag == XDS_GIFT);

	/* Free the old buffer if there is one. */
	if (xds->buffer != NULL && xds->we_own_buffer)
		(mqlib_free) (xds->buffer);
	xds->buffer_len = 0;

	if (flag == XDS_GIFT) {
		xds->buffer = buffer;
		if (buffer == NULL)
			xds->buffer_capacity = 0;
		else
			xds->buffer_capacity = buffer_len;
		xds->we_own_buffer = XDS_TRUE;
	} else {
		xds->buffer = buffer;
		xds->buffer_capacity = buffer_len;
		xds->we_own_buffer = XDS_FALSE;
	}

	return XDS_OK;
}

int
xds_getbuffer (xds_t * xds, xds_scope_t flag, void **buffer, size_t * buffer_len)
{
	/* Sanity check parameters. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (flag == XDS_GIFT || flag == XDS_LOAN);
	xds_check_parameter (buffer != NULL);
	xds_check_parameter (buffer_len != NULL);

	/* Return the buffer to the caller. */
	*buffer = xds->buffer;
	*buffer_len = xds->buffer_len;
	if (flag == XDS_GIFT) {
		xds->buffer = NULL;
		xds->buffer_capacity = 0;
		xds->buffer_len = 0;
	} else
		xds->buffer_len = 0;

	return XDS_OK;
}

static int
xds_set_capacity (void **array, size_t * array_capacity, size_t new_capacity, size_t elem_size, size_t initial_capacity)
{
	void *buf;
	size_t size;

	/* Sanity checks. */
	xds_check_parameter (array != NULL);
	xds_check_parameter (array_capacity != NULL);
	xds_check_parameter (elem_size != 0);
	xds_check_parameter (initial_capacity != 0);

	/* Do we need to re-allocate? */
	if (*array_capacity > new_capacity)
		return XDS_OK;

	/* Find the correct capacity. */
	size = (*array_capacity != 0) ? (*array_capacity * 2) : initial_capacity;
	while (size < new_capacity)
		size *= 2;

	/* Allocate the array and store the new values. */
	if ((buf = realloc (*array, size * elem_size)) == NULL)
		return XDS_ERR_NO_MEM;
	*array = buf;
	*array_capacity = size;

	return XDS_OK;
}

static int
xds_find_engine (const engine_map_t * engines, size_t last, const char *name, size_t * pos)
{
	size_t first;

	/* Sanity checks. */
	xds_check_parameter (engines != NULL || last == 0);
	xds_check_parameter (name != NULL);
	xds_check_parameter (pos != NULL);

	/* Use binary search to find "name" in "engines". */
	for (first = 0; (last - first) > 0;) {
		size_t half = first + ((last - first) / 2);
		int rc = strcmp (engines[half].name, name);
		if (rc < 0)
			first = half + 1;
		else if (rc == 0) {	/* found it */
			*pos = half;
			return XDS_TRUE;
		} else
			last = half;
		assert (first <= last);
	}
	*pos = first;

	return XDS_FALSE;
}

int
xds_register (xds_t * xds, const char *name, xds_engine_t engine, void *engine_context)
{
	size_t pos;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (name != NULL);
	xds_check_parameter (engine != NULL);
	for (pos = 0; name[pos] != '\0'; ++pos) {
		if (!isalnum ((int) name[pos]) && name[pos] != '-' && name[pos] != '_')
			return XDS_ERR_INVALID_ARG;
	}

	/* Copy the name argument into our own memory. */
	name = strdup (name);
	if (name == NULL)
		return XDS_ERR_NO_MEM;

	/* Search engines for the entry. */
	if (xds_find_engine (xds->engines, xds->engines_len, name, &pos)) {
		/* overwrite existing entry */
		(mqlib_free) (xds->engines[pos].name);
	} else {
		/* insert new entry */
		int rc = xds_set_capacity ((void **) &xds->engines,
					   &xds->engines_capacity,
					   xds->engines_len + 1,
					   sizeof (engine_map_t),
					   XDS_INITIAL_ENGINES_CAPACITY);
		assert (rc == XDS_OK || rc == XDS_ERR_NO_MEM);
		if (rc != XDS_OK)
			return rc;
		memmove (&xds->engines[pos + 1], &xds->engines[pos], (xds->engines_len - pos) * sizeof (engine_map_t));
		xds->engines_len++;
	}

	/* Insert entry. */
	xds->engines[pos].name = (char *) name;
	xds->engines[pos].engine = engine;
	xds->engines[pos].context = engine_context;

	/* Everything is fine. */
	return XDS_OK;
}

int
xds_unregister (xds_t * xds, const char *name)
{
	size_t pos;
	int rc;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (name != NULL);
	for (pos = 0; name[pos] != '\0'; pos++) {
		if (!isalnum ((int) name[pos]) && name[pos] != '-' && name[pos] != '_')
			return XDS_ERR_INVALID_ARG;
	}

	/* Find the entry we're supposed to delete. */
	if (!xds_find_engine (xds->engines, xds->engines_len, name, &pos))
		return XDS_ERR_UNKNOWN_ENGINE;

	/* Free the memory allocated for this entry and move the entries behind
	   it back if necessary. */
	assert (xds->engines[pos].name != NULL);
	(mqlib_free) (xds->engines[pos].name);
	memmove (&xds->engines[pos], &xds->engines[pos + 1], (xds->engines_len - (pos + 1)) * sizeof (engine_map_t));
	xds->engines_len--;

	/* Lower capacity if necessary. */
	rc = xds_set_capacity ((void **) &xds->engines, &xds->engines_capacity, xds->engines_len, sizeof (engine_map_t), XDS_INITIAL_ENGINES_CAPACITY);
	assert (rc == XDS_OK || rc == XDS_ERR_NO_MEM);
	if (rc != XDS_OK)
		return rc;

	return XDS_OK;
}

int
xds_encode (xds_t * xds, const char *fmt, ...)
{
	int rc;
	va_list args;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (fmt != NULL);

	va_start (args, fmt);
	rc = xds_vencode (xds, fmt, args);
	va_end (args);
	return rc;
}

int
xds_vencode (xds_t * xds, const char *fmt_arg, va_list args)
{
	va_list args_backup;
	size_t buffer_len_backup;
	char *name;
	char *p;
	char *fmt;
	int rc;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (fmt_arg != NULL);
	assert (xds->mode == XDS_ENCODE);
	if (xds->mode != XDS_ENCODE)
		return XDS_ERR_INVALID_MODE;

	/* Ensure we have a buffer allocated ready for use. */
	if (xds->buffer == NULL) {
		/* allocate a new buffer */
		rc = xds_set_capacity ((void **) &xds->buffer, &xds->buffer_capacity, XDS_INITIAL_BUFFER_CAPACITY, sizeof (char), XDS_INITIAL_BUFFER_CAPACITY);
		assert (rc == XDS_OK || rc == XDS_ERR_NO_MEM);
		if (rc != XDS_OK)
			return rc;
		xds->buffer_len = 0;
		xds->we_own_buffer = XDS_TRUE;
	}

	/* Iterate through items in format string and execute apropriate engines. */
	fmt = p = strdup (fmt_arg);
	if (fmt == NULL)
		return XDS_ERR_NO_MEM;
	buffer_len_backup = xds->buffer_len;
	for (name = p; *p != '\0'; name = p) {
		while (isalnum ((int) *p) || *p == '-' || *p == '_')
			p++;
		if (*p != '\0')
			*p++ = '\0';
		else
			*p = '\0';

		if (strlen (name) > 0) {
			int restart_engine;
			size_t used_buffer_size;
			size_t pos;

			if (xds_find_engine (xds->engines, xds->engines_len, name, &pos)
			    == XDS_FALSE) {
				rc = XDS_ERR_UNKNOWN_ENGINE;
				goto leave;
			}

			do {
				/* Ensure the buffer has free space. */
				assert (xds->buffer_len <= xds->buffer_capacity);
				if (xds->buffer_len == xds->buffer_capacity) {
					if (xds->we_own_buffer) {
						rc = xds_set_capacity ((void **) &xds->buffer, &xds->buffer_capacity, xds->buffer_len + 1, sizeof (char), XDS_INITIAL_BUFFER_CAPACITY);
						assert (rc == XDS_OK || rc == XDS_ERR_NO_MEM);
						if (rc != XDS_OK)
							goto leave;
					} else {
						rc = XDS_ERR_OVERFLOW;
						goto leave;
					}
				}

				/* Execute the engine. */
				used_buffer_size = 0;
				args_backup = args;
				rc = (*xds->engines[pos].engine) (xds, xds->engines[pos].context, xds->buffer + xds->buffer_len, xds->buffer_capacity - xds->buffer_len, &used_buffer_size, &args);
				assert (rc <= 0);
				if (rc == XDS_OK) {
					restart_engine = XDS_FALSE;
					xds->buffer_len += used_buffer_size;
				} else if (rc == XDS_ERR_OVERFLOW) {	/* enlarge buffer */
					if (!xds->we_own_buffer)
						goto leave;

					restart_engine = XDS_TRUE;
					args = args_backup;

					rc = xds_set_capacity ((void **) &xds->buffer, &xds->buffer_capacity,
							       xds->buffer_capacity + ((used_buffer_size == 0) ? 1 : used_buffer_size), sizeof (char), XDS_INITIAL_BUFFER_CAPACITY);
					assert (rc == XDS_OK || rc == XDS_ERR_NO_MEM);
					if (rc != XDS_OK)
						goto leave;
				} else
					goto leave;
			}
			while (restart_engine);
		}
	}
	rc = XDS_OK;

	/* Clean up and leave. */
      leave:
	(mqlib_free) (fmt);
	if (rc != XDS_OK)
		xds->buffer_len = buffer_len_backup;
	return rc;
}

int
xds_decode (xds_t * xds, const char *fmt, ...)
{
	int rc;
	va_list args;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (fmt != NULL);

	va_start (args, fmt);
	rc = xds_vdecode (xds, fmt, args);
	va_end (args);
	return rc;
}

int
xds_vdecode (xds_t * xds, const char *fmt_arg, va_list args)
{
	size_t buffer_len_backup;
	char *name;
	char *p;
	char *fmt;
	int rc;

	/* Sanity checks. */
	xds_check_parameter (xds != NULL);
	xds_check_parameter (fmt_arg != NULL);
	assert (xds->mode == XDS_DECODE);
	if (xds->mode != XDS_DECODE)
		return XDS_ERR_INVALID_MODE;

	/* Ensure we have a buffer to decode. */
	if (xds->buffer == NULL || xds->buffer_capacity == 0)
		return XDS_ERR_UNDERFLOW;

	/* Iterate through the items in the format string and execute the
	   apropriate engines. */
	fmt = p = strdup (fmt_arg);
	if (fmt == NULL)
		return XDS_ERR_NO_MEM;
	buffer_len_backup = xds->buffer_len;
	for (name = p; *p != '\0'; name = p) {
		while (isalnum ((int) *p) || *p == '-' || *p == '_')
			p++;
		if (*p != '\0')
			*p++ = '\0';
		else
			*p = '\0';

		if (strlen (name) > 0) {
			size_t pos;
			size_t used_buffer_size = 0;

			if (xds_find_engine (xds->engines, xds->engines_len, name, &pos)) {
				rc = (*xds->engines[pos].engine) (xds, xds->engines[pos].context, xds->buffer + xds->buffer_len, xds->buffer_capacity - xds->buffer_len, &used_buffer_size, &args);
				assert (rc <= 0);
				if (rc == XDS_OK)
					xds->buffer_len += used_buffer_size;
				else
					goto leave;
			} else {
				rc = XDS_ERR_UNKNOWN_ENGINE;
				goto leave;
			}
		}
	}
	rc = XDS_OK;

	/* Clean up and leave. */
      leave:
	(mqlib_free) (fmt);
	if (rc != XDS_OK)
		xds->buffer_len = buffer_len_backup;
	return rc;
}

int
xds_get_usedbuffer (xds_t * xds)
{
	return xds->buffer_len;
}
