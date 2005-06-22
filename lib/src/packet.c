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

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "defines.h"
#include "list.h"
#include "packet.h"
#include "xds.h"
#include "buffer.h"


int close_fd (mqp *mqplib, mqpacket * mqp);

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


/* this setups our standard malloc/free routines */
void *(*mqlib_malloc)(size_t) = malloc;
void (*mqlib_free)(void *) = free;



mqp *init_mqlib () {

	mqp *mqplib;
	
	mqplib = (mqlib_malloc)(sizeof(mqp));
	mqplib->logger = NULL;
	mqplib->callback = NULL;
	mqplib->myengines = standeng;
	return mqplib;
}


void fini_mqlib (mqp *mqplib) {
	(mqlib_free)(mqplib);
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


xds_t * 
pck_init_engines (mqp *mqplib, int type, int direction) {
	int i, rc = 0;
	xds_t *xds;
	if (xds_init (&xds, direction) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds init failed: %s", strerror (errno));
		return NULL;
	}
	i = 0;
	while (i < NUMENGINES) {
		if (mqplib->myengines[i].type == type) {
			if (mqplib->myengines[i].dir == direction) {
				rc = xds_register (xds, mqplib->myengines[i].myname, mqplib->myengines[i].ptr, NULL);
			}
			if (rc != XDS_OK) {
				if (mqplib->logger)
					mqplib->logger ("xds_register failed for %s", mqplib->myengines[i].myname);
				xds_destroy (xds);
				return NULL;
			}
		}
		i++;
	}
	return xds;
}

mqpacket *
pck_new_connection (mqp *mqplib, int fd, int type, int contype)
{
	mqpacket *mqpck;
	
	mqpck = (mqlib_malloc) (sizeof (mqpacket));
	mqpck->sock = fd;
	mqpck->outmsg.MID = -1;
	mqpck->nxtoutmid = 1;
	mqpck->xdsin = pck_init_engines(mqplib, type, XDS_DECODE);
	mqpck->xdsout = pck_init_engines(mqplib, type, XDS_ENCODE);
	mqpck->pollopts = 0;
	mqpck->si.username = NULL;
	mqpck->si.password = NULL;
	mqpck->inbuf = mqbuffer_new();
	mqpck->outbuf = mqbuffer_new();
	

#if 0
	if (xds_init (&mqpck->xdsin, XDS_DECODE) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds init failed: %s", strerror (errno));
		(mqlib_free) (mqpck);
		return NULL;
	}
	if (xds_init (&mqpck->xdsout, XDS_ENCODE) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds init failed: %s", strerror (errno));
		(mqlib_free) (mqpck);
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
				(mqlib_free) (mqpck);
				mqpck = NULL;
			}
		}
		i++;
	}
#endif		
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
	if (mqpck->data) (mqlib_free) (mqpck->data);
#endif
	xds_destroy (mqpck->xdsout);
	xds_destroy (mqpck->xdsin);
	mqbuffer_free(mqpck->inbuf);
	mqbuffer_free(mqpck->outbuf);
	if (mqpck->si.username) {
		(mqlib_free)(mqpck->si.username);
		(mqlib_free)(mqpck->si.password);
	}
	(mqlib_free) (mqpck);

}

int
read_fd (mqp *mqplib, mqpacket *mqp)
{
	char buf[BUFSIZE];
	int i, used;


	bzero (buf, BUFSIZE);
	i = read (mqp->sock, buf, BUFSIZE);
	if ((i < 0) && (i != EAGAIN)) {
		/* error */
		if (mqplib->logger)
			mqplib->logger("Failed to Read fd %d(%d): %s", mqp->sock,i, strerror(errno));
		close_fd (mqplib, mqp);
		/* XXX close and clean up */
		return NS_FAILURE;
	} else if (i == 0) {
		return NS_FAILURE;
	} else {
		mqbuffer_add(mqp->inbuf, buf, i);
#if 0
		buffer_add (mqp, buf, i);
#endif
	}
	/* XXX don't process packet till its authed */
	while (mqp->inbuf->off > 0) {
		used = pck_parse_packet (mqplib, mqp);
		if (used > 1) {
			mqbuffer_drain (mqp->inbuf, used);
		} else if (used == NS_FAILURE) {
			close_fd (mqplib, mqp);
			return NS_FAILURE;
		} else if (used == -2) {
			return NS_SUCCESS;
		}
	}
	if (mqplib->logger) {
		mqplib->logger ("processed %d bytes %d left", i, mqp->inbuf->off);
	}
	return NS_SUCCESS;
}

int
close_fd (mqp *mqplib, mqpacket * mqp)
{
	if (mqplib->logger) {
		mqplib->logger ("Closing %d fd", mqp->sock);
	}
	close (mqp->sock);
	pck_del_connection (mqplib, mqp);

	return NS_SUCCESS;
}


int
write_fd (mqp *mqplib, mqpacket * mqp)
{
	int i;
	int static j = 0;	
#ifdef DEBUG
	if (j == 1) {
		print_decode(mqp,2);
		j = 0;
	}
#endif
	if (mqp->outbuf->off > 0) {
		i = write (mqp->sock, mqp->outbuf->buffer, mqp->outbuf->off);
		if (i < 0 ) {
			if (errno == EAGAIN) {
				return NS_SUCCESS;
			}
			/* something went wrong sending the data */
			if (mqplib->logger)
				mqplib->logger ("Error Sending on fd %d(%d): %s", mqp->sock, i, strerror(errno));
			close_fd (mqplib, mqp);
			return NS_FAILURE;
		} else if (i == 0) {
			return NS_SUCCESS;
		} else {
			mqbuffer_drain(mqp->outbuf, i);
			if (i == mqp->outbuf->off) {
				mqp->pollopts |= POLLOUT;
			} else {
				mqp->pollopts &= ~POLLOUT;
			}
		}
#if 0
		if (i == mqp->outbuff->off) {
			bzero(mqp->outbuffer, mqp->outbufferlen);
			(mqlib_free) (mqp->outbuffer);
			mqp->outbufferlen = mqp->outoffset = 0;
			mqp->pollopts &= ~POLLOUT;
		} else if (i > 0) {
			memmove (mqp->outbuffer, mqp->outbuffer + i, mqp->outbufferlen - i);
			mqp->outbufferlen -= i;
			mqp->pollopts |= POLLOUT;
			print_decode(mqp,2);
			j = 1;
		} else {
		}
#endif
	} else {
		/* somethign went wrong encoding the data */
		if (mqplib->logger)
			mqplib->logger ("No Data to Send?");
#if 0
		close_fd (mqplib, mqp);
		return NS_FAILURE;
#endif
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
			strncpy(buf2, mqp->inbuf->buffer, mqp->inbuf->off);
			len = mqp->inbuf->off;
			break;
		case 2:
			printf("Encode: ");
			strncpy(buf2, mqp->outbuf->buffer, mqp->outbuf->off);
			len = mqp->outbuf->off;
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
