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

/* needed for crc32 function */
#include <zlib.h>

#include "defines.h"
#include "list.h"
#include "packet.h"
#include "xds.h"


/* this is called from the socket recv code. */
int
pck_parse_packet (mqp *mqplib, mqpacket * mqp, u_char * buffer, unsigned long buflen)
{
	int rc, usedbuf;

	print_decode(mqp, 1);
	
	/* XXX XDR engine for now */
	if (xds_setbuffer (mqp->xdsin, XDS_LOAN, buffer, buflen) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("XDS setbuffer Failed");
		/* XXX drop client ? */
		return NS_FAILURE;
	}
	/* lets use a switch here to handle it intelegently */

	if (mqp->wiretype == ENG_TYPE_XML) {
		rc = xds_decode (mqp->xdsin, "xmlstart mqpheader", mqp);
	} else {
		rc = xds_decode (mqp->xdsin, "mqpheader", mqp);
	}
	switch (rc) {
		case XDS_OK:
			usedbuf = xds_get_usedbuffer (mqp->xdsin);
			break;
		case XDS_ERR_UNDERFLOW:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Header Failed. Buffer Underflow");
			/* don't consume any buffer */
			return NS_SUCCESS;
		default:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Header Failed: %d", rc);
			/* drop client */
			return NS_FAILURE;
	}
	if (mqplib->logger)
		mqplib->logger ("Got Packet Decode: mid %d msgtype %d Version %d flags %d ", mqp->inmsg.MID, mqp->inmsg.MSGTYPE, mqp->inmsg.VERSION, mqp->inmsg.flags);
		/* check the version number */
	if (mqp->inmsg.VERSION != 1) {
		if (mqplib->logger)
			mqplib->logger ("Invalid Protocol Version recieved");
		/* XXX drop client ? */
		return NS_FAILURE;
	}
	/* ok, lets handle this message based on the message type */
	switch (mqp->inmsg.MSGTYPE) {
		case PCK_ACK:
			rc = xds_decode(mqp->xdsin, PCK_ACK_FMT, &mqp->inmsg.data.num);
			break;
		case PCK_ERROR:
			rc = xds_decode(mqp->xdsin, PCK_ERROR_FMT, &mqp->inmsg.data.string);
			break;
		case PCK_SRVCAP:
			rc = xds_decode(mqp->xdsin, PCK_SRVCAP_FMT, &mqp->inmsg.data.srvcap.srvcap1, &mqp->inmsg.data.srvcap.srvcap2, &mqp->inmsg.data.srvcap.capstr);
			break;
		case PCK_CLNTCAP:
			rc = xds_decode(mqp->xdsin, PCK_SRVCAP_FMT, &mqp->inmsg.data.srvcap.srvcap1, &mqp->inmsg.data.srvcap.srvcap2, &mqp->inmsg.data.srvcap.capstr);
			break;
		case PCK_AUTH:
			rc = xds_decode(mqp->xdsin, PCK_AUTH_FMT, &mqp->inmsg.data.auth.username, &mqp->inmsg.data.auth.password);
			break;
		case PCK_SENDTOQUEUE:
			rc = xds_decode(mqp->xdsin, PCK_SENDTOQUEUE_FMT, &mqp->inmsg.data.stream.queue, &mqp->inmsg.data.stream.len, &mqp->inmsg.data.stream.data, &mqp->inmsg.data.stream.datalen);
			break;
		case PCK_JOINQUEUE:
			rc = xds_decode(mqp->xdsin, PCK_JOINQUEUE_FMT, &mqp->inmsg.data.joinqueue.queue, &mqp->inmsg.data.joinqueue.flags);
			break;
		default:
			if (mqplib->logger)
				mqplib->logger ("Invalid MsgType Recieved");
			/* XXX drop client */
			return NS_FAILURE;
	}
	switch (rc) {
		case XDS_OK:
			usedbuf = xds_get_usedbuffer (mqp->xdsin);
			break;
		case XDS_ERR_UNDERFLOW:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Data Failed. Buffer Underflow");
			/* don't consume any buffer */
			return NS_SUCCESS;
		default:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Data Failed: %d", rc);
			/* drop client */
			return NS_FAILURE;
	}
	if (mqp->wiretype == ENG_TYPE_XML) {
		rc = xds_decode (mqp->xdsin, "xmlstop", mqp);
		switch (rc) {
			case XDS_OK:
				usedbuf = xds_get_usedbuffer (mqp->xdsin);
				break;
			case XDS_ERR_UNDERFLOW:
				if (mqplib->logger)
					mqplib->logger ("XDS Decode of Header Failed. Buffer Underflow");
				/* don't consume any buffer */
				return NS_SUCCESS;
			default:
				if (mqplib->logger)
					mqplib->logger ("XDS Decode of Header Failed: %d", rc);
				/* drop client */
				return NS_FAILURE;
		}
	}
	if (mqplib->callback) {
		mqplib->callback((void *)mqplib, mqp);
	}

	/* finished processing the message. grab what buffer we consumed and return */
	return usedbuf;
}

