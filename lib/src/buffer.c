/* MQServer - Abstract message Processing Server
** Copyright (c) 2005 Justin Hammond
** http://www.mqserver.info/
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
**  USA
**
** MQServer CVS Identification
** $Id$
*/

/*
 * Copyright (c) 2002, 2003 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "packet.h"
#include "buffer.h"


struct mqbuffer *
mqbuffer_new(void)
{
	struct mqbuffer *buffer;
	
	buffer = calloc(1, sizeof(struct mqbuffer));

	return (buffer);
}

void
mqbuffer_free(struct mqbuffer *buffer)
{
	if (buffer->buffer != NULL)
		(mqlib_free)(buffer->buffer);
	(mqlib_free)(buffer);
}

/* 
 * This is a destructive add.  The data from one buffer moves into
 * the other buffer.
 */

int
mqbuffer_add_buffer(struct mqbuffer *outbuf, struct mqbuffer *inbuf)
{
	int res;
	res = mqbuffer_add(outbuf, inbuf->buffer, inbuf->off);
	if (res == 0)
		mqbuffer_drain(inbuf, inbuf->off);

	return (res);
}

int
mqbuffer_add_printf(struct mqbuffer *buf, char *fmt, ...)
{
	int res = -1;
	char *msg;
	va_list ap;

	va_start(ap, fmt);

	if (vasprintf(&msg, fmt, ap) == -1)
		goto end;
	
	res = strlen(msg);
	if (mqbuffer_add(buf, msg, res) == -1)
		res = -1;
	(mqlib_free)(msg);

 end:
	va_end(ap);

	return (res);
}

int
mqbuffer_add(struct mqbuffer *buf, u_char *data, size_t datlen)
{
	size_t need = buf->off + datlen;
	size_t oldoff = buf->off;

	if (buf->totallen < need) {
		void *newbuf;
		int length = buf->totallen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if ((newbuf = realloc(buf->buffer, length)) == NULL)
			return (-1);

		buf->buffer = newbuf;
		buf->totallen = length;
	}

	memcpy(buf->buffer + buf->off, data, datlen);
	buf->off += datlen;
 	buf->buffer[buf->off] = '\0'; 
#ifdef PACKDEBUG
printf("MyBUF\n%s\nENDBUF\n", buf->buffer);
printf("LASTCHAR: %c (%d)\n", buf->buffer[buf->off], buf->buffer[buf->off]);
#endif
	if (datlen && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

	return (0);
}

void
mqbuffer_drain(struct mqbuffer *buf, size_t len)
{
	size_t oldoff = buf->off;

	if (len >= buf->off) {
		buf->off = 0;
		goto done;
	}

	memmove(buf->buffer, buf->buffer + len, (buf->off+1) - len);
	buf->off -= len;

 done:
	/* Tell someone about changes in this buffer */
	if (buf->off != oldoff && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

}

int
mqbuffer_read(struct mqbuffer *buffer, int fd, int howmuch)
{
	u_char inbuf[4096];
	int n;
	
	if (howmuch < 0 || howmuch > sizeof(inbuf))
		howmuch = sizeof(inbuf);

	n = read(fd, inbuf, howmuch);
	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);

	mqbuffer_add(buffer, inbuf, n);

	return (n);
}

int
mqbuffer_write(struct mqbuffer *buffer, int fd)
{
	int n;

	n = write(fd, buffer->buffer, buffer->off);
	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);

	mqbuffer_drain(buffer, n);

	return (n);
}

u_char *
mqbuffer_find(struct mqbuffer *buffer, u_char *what, size_t len)
{
	size_t remain = buffer->off;
	u_char *search = buffer->buffer;
	u_char *p;

	while ((p = memchr(search, *what, remain)) != NULL && remain >= len) {
		if (memcmp(p, what, len) == 0)
			return (p);

		search = p + 1;
		remain = buffer->off - (size_t)(search - buffer->buffer);
	}

	return (NULL);
}

void mqbuffer_setcb(struct mqbuffer *buffer,
    void (*cb)(struct mqbuffer *, size_t, size_t, void *),
    void *cbarg)
{
	buffer->cb = cb;
	buffer->cbarg = cbarg;
}
