/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond
** http://www.neostats.net/
**
**  Portions Copyright (c) 2000-2001 ^Enigma^
**
**  Portions Copyright (c) 1999 Johnathan George net@lite.net
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
** NeoStats CVS Identification
** $Id: client.c 4 2004-04-28 12:02:07Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defines.h"
#include "list.h"
#include "packet.h"
#include "xds.h"


int close_fd (mqp *mqplib, mqpacket * mqp);
int buffer_add (mqpacket * mqp, void *data, size_t datlen);
void buffer_del (mqpacket * mqp, size_t drainlen);



myeng standeng[] = {
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_uint32, "uint32" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_int32, "int32" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_uint64, "unit64" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_int64, "int64" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_float, "float" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_double, "double" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_octetstream, "octet" },
	{ XDS_ENCODE, ENG_TYPE_XDR, xdr_encode_string, "string" },
	{ XDS_ENCODE, ENG_TYPE_XDR, encode_mqs_header, "mqpheader" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_uint32, "uint32" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_int32, "int32" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_uint64, "unit64" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_int64, "int64" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_float, "float" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_double, "double" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_octetstream, "octet" },
	{ XDS_DECODE, ENG_TYPE_XDR, xdr_decode_string, "string" },
	{ XDS_DECODE, ENG_TYPE_XDR, decode_mqs_header, "mqpheader" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_begin, "xmlstart" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_uint32, "uint32" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_int32, "int32" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_uint64, "unit64" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_int64, "int64" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_float, "float" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_double, "double" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_octetstream, "octet" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_string, "string" },
	{ XDS_ENCODE, ENG_TYPE_XML, encode_mqs_header, "mqpheader" },
	{ XDS_ENCODE, ENG_TYPE_XML, xml_encode_end, "xmlstop" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_begin, "xmlstart" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_uint32, "uint32" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_int32, "int32" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_uint64, "unit64" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_int64, "int64" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_float, "float" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_double, "double" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_octetstream, "octet" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_string, "string" },
	{ XDS_DECODE, ENG_TYPE_XML, decode_mqs_header, "mqpheader" },
	{ XDS_DECODE, ENG_TYPE_XML, xml_decode_end, "xmlstop" },
};




mqp *init_mqlib () {

	mqp *mqplib;
	
	mqplib = malloc(sizeof(mqp));
	mqplib->logger = NULL;
	mqplib->callback = NULL;
	mqplib->myengines = standeng;
	return mqplib;
}

void
pck_set_logger (mqp *mqplib, logfunc * logger)
{
	mqplib->logger = logger;
}


void
pck_set_callback(mqp *mqplib, callbackfunc *callback) {
	mqplib->callback = callback;
}

void 
pck_set_authcallback(mqp *mqplib, connectauthfunc *ca) {
	mqplib->connectauth = ca;
}

void pck_set_data(mqpacket *mqp, void *data) {
	mqp->cbarg = data;
}


mqpacket *
pck_new_connection (mqp *mqplib, int fd, int type, int contype)
{
	int i, rc;
	mqpacket *mqpck;
	
	mqpck = malloc (sizeof (mqpacket));
	mqpck->offset = mqpck->outoffset = 0;
	mqpck->sock = fd;
	mqpck->outmsg.MID = -1;
	mqpck->nxtoutmid = 1;
		
	if (xds_init (&mqpck->xdsin, XDS_DECODE) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds init failed: %s", strerror (errno));
		free (mqpck);
		return NULL;
	}
	if (xds_init (&mqpck->xdsout, XDS_ENCODE) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds init failed: %s", strerror (errno));
		free (mqpck);
		return NULL;
	}
	i = 0;
	while (i < NUMENGINES) {
		if (mqplib->myengines[i].type == type) {
			if (mqplib->myengines[i].dir == XDS_ENCODE) {
				rc = xds_register (mqpck->xdsout, mqplib->myengines[i].myname, mqplib->myengines[i].ptr, NULL);
			} else {
				rc = xds_register (mqpck->xdsin, mqplib->myengines[i].myname, mqplib->myengines[i].ptr, NULL);
			}
			if (rc != XDS_OK) {
				if (mqplib->logger)
					mqplib->logger ("xds_register failed for %s", mqplib->myengines[i].myname);
				xds_destroy (mqpck->xdsout);
				xds_destroy (mqpck->xdsin);
				free (mqpck);
				mqpck = NULL;
			}
		}
		i++;
	}
	mqpck->wiretype = type;	
	switch (contype) {
		case PCK_IS_CLIENT:
			pck_send_srvcap(mqplib, mqpck);
			break;
		case PCK_IS_SERVER:
			break;
		default:
			if (mqplib->logger) 
				mqplib->logger ("pck_new_connection invalid type %d", contype);
			break;
	}
	return mqpck;
}

void
pck_del_connection (mqp *mqplib, mqpacket * mqpck)
{

#if 0
	if (mqpck->data) free (mqpck->data);
#endif
	xds_destroy (mqpck->xdsout);
	xds_destroy (mqpck->xdsin);
	if (mqpck->si.username) {
		free(mqpck->si.username);
		free(mqpck->si.password);
	}
	free (mqpck);

}

int
read_fd (mqp *mqplib, mqpacket *mqp)
{
	char buf[BUFSIZE];
	int i;


	bzero (buf, BUFSIZE);
	i = read (mqp->sock, buf, BUFSIZE);
	if ((i <= 0) && (i != EAGAIN)) {
		/* error */
		if (mqplib->logger)
			mqplib->logger("Failed to Read fd %d: %s", mqp->sock, strerror(errno));
		close_fd (mqplib, mqp);
		/* XXX close and clean up */
		return NS_FAILURE;
	} else {
		buffer_add (mqp, buf, i);
	}
	/* don't process packet till its authed */
	while ((mqp->offset > 0)  && i > 0) {
		i = pck_parse_packet (mqplib, mqp, mqp->buffer, mqp->offset);
		if (i > 1) {
			buffer_del (mqp, i);
		} else if (i == NS_FAILURE) {
			close_fd (mqplib, mqp);
			return NS_FAILURE;
		} else if (i == -2) {
			return NS_SUCCESS;
		}
	}
	if (mqplib->logger) {
		mqplib->logger ("processed %d bytes %d left", i, mqp->offset);
	}
	return NS_SUCCESS;
}

int
close_fd (mqp *mqplib, mqpacket * mqp)
{
	if (mqplib->logger) {
		mqplib->logger ("Closing %d fd", mqp->sock);
	}
	pck_del_connection (mqplib, mqp);

	close (mqp->sock);
	return NS_SUCCESS;
}


int
buffer_add (mqpacket * mqp, void *data, size_t datlen)
{
	size_t need = mqp->offset + datlen;
//      size_t oldoff = mqp->offset;

	if (mqp->bufferlen < need) {
		void *newbuf;
		int length = mqp->bufferlen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if ((newbuf = realloc (mqp->buffer, length)) == NULL)
			return (-1);
		mqp->buffer = newbuf;
		mqp->bufferlen = length;
	}

	memcpy (mqp->buffer + mqp->offset, data, datlen);
	mqp->offset += datlen;

#if 0
	if (datlen && mqp->cb != NULL)
		(*mqp->cb) (mqp, oldoff, mqp->off, mqp->cbarg);
#endif
	return (0);
}

void
buffer_del (mqpacket * mqp, size_t drainlen)
{

	memmove (mqp->buffer, mqp->buffer + drainlen, mqp->offset - drainlen);
	mqp->offset = mqp->offset - drainlen;
}

int
write_fd (mqp *mqplib, mqpacket * mqp)
{
	int i;
	
	if (mqp->outbufferlen > 0) {
		i = write (mqp->sock, mqp->outbuffer, mqp->outbufferlen);
		if (i == mqp->outbufferlen) {
			bzero(mqp->outbuffer, mqp->outbufferlen);
			free (mqp->outbuffer);
			mqp->outbufferlen = mqp->outoffset = 0;
			mqp->pollopts &= ~POLLOUT;
		} else if (i > 0) {
			memmove (mqp->outbuffer, mqp->outbuffer + i, mqp->outoffset - i);
			mqp->outoffset = mqp->outoffset - i;
			mqp->pollopts |= POLLOUT;
		} else {
			/* something went wrong sending the data */
			if (mqplib->logger)
				mqplib->logger ("Error Sending on fd %d", mqp->sock);
			close_fd (mqplib, mqp);
			return NS_FAILURE;
		}
	} else {
		/* somethign went wrong encoding the data */
		if (mqplib->logger)
			mqplib->logger ("No Data to Send?");
		close_fd (mqplib, mqp);
		return NS_FAILURE;
	}
	return NS_SUCCESS;
}


void print_decode(mqpacket *mqp, int what) {
	int i;
	char buf2[1024];
	int len = 0;
	
	bzero(buf2, 1024);
	switch (what) {
		case 1:
			printf("Decode: ");
			strncpy(buf2, mqp->buffer, mqp->offset);
			len = mqp->offset;
			break;
		case 2:
			printf("Encode: ");
			strncpy(buf2, mqp->outbuffer, mqp->outbufferlen);
			len = mqp->outbufferlen;
			break;

	}
	switch (mqp->wiretype) {
		case ENG_TYPE_XDR:
			for (i=0; i < len; i++) {
				printf("%x ", buf2[i]);
			}
			break;
		case ENG_TYPE_XML:
			printf("%s", (char *)buf2);
			break;
	}
	printf("\n");
}
