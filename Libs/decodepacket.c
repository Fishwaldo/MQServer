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
pck_parse_packet (mqp *mqplib, mqpacket * mqp)
{
	int rc, usedbuf;

	
	/* XXX XDR engine for now */
	printf("%d\n", mqp->inbuf->off);
	if (xds_setbuffer (mqp->xdsin, XDS_LOAN, mqp->inbuf->buffer, mqp->inbuf->off) != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("XDS setbuffer Failed");
		/* XXX drop client ? */
		return NS_FAILURE;
	}
	/* lets use a switch here to handle it intelegently */

	if (mqp->wiretype == ENG_TYPE_XML) {
		/* look for terminator */
		/* XML is a string, so make sure its null terminated at the right point */
/*		strcat(buffer, '\0'); */
/*		buffer[buflen] = '\0'; */
		if (!strstr(mqp->inbuf->buffer, "</xds>")) {
			return -2;
		} 
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
			return -2;
		default:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Header Failed: %d", rc);
			print_decode(mqp, 1);
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
			rc = xds_decode(mqp->xdsin, PCK_SENDTOQUEUE_FMT, &mqp->inmsg.data.stream.queue, &mqp->inmsg.data.stream.len, &mqp->inmsg.data.stream.topic, &mqp->inmsg.data.stream.data, &mqp->inmsg.data.stream.datalen);
			break;
		case PCK_JOINQUEUE:
			rc = xds_decode(mqp->xdsin, PCK_JOINQUEUE_FMT, &mqp->inmsg.data.joinqueue.queue, &mqp->inmsg.data.joinqueue.flags, &mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_QUEUEINFO:
			rc = xds_decode(mqp->xdsin, PCK_QUEUEINFO_FMT, &mqp->inmsg.data.joinqueue.queue, &mqp->inmsg.data.joinqueue.flags, &mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_MSGFROMQUEUE:
			rc = xds_decode(mqp->xdsin, PCK_MSGFROMQUEUE_FMT, &mqp->inmsg.data.sendmsg.queue, &mqp->inmsg.data.sendmsg.topic, &mqp->inmsg.data.sendmsg.data, &mqp->inmsg.data.sendmsg.len, &mqp->inmsg.data.sendmsg.messid, &mqp->inmsg.data.sendmsg.timestamp, &mqp->inmsg.data.sendmsg.from);
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
			return -2;
		default:
			if (mqplib->logger)
				mqplib->logger ("XDS Decode of Data Failed: %d", rc);
			/* drop client */
			print_decode(mqp, 1);
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
					mqplib->logger ("XDS Decode of Footer Failed. Buffer Underflow");
				/* don't consume any buffer */
				return -2;
			default:
				if (mqplib->logger)
					mqplib->logger ("XDS Decode of Footer Failed: %d", rc);
				/* drop client */
				print_decode(mqp, 1);
				return NS_FAILURE;
		}
	}
	if (mqplib->callback) {
		mqplib->callback((void *)mqplib, mqp);
	}
	switch (mqp->inmsg.MSGTYPE) {
		case PCK_ACK:
			break;
		case PCK_ERROR:
			free(mqp->inmsg.data.string);
			break;
		case PCK_SRVCAP:
		case PCK_CLNTCAP:
			free(mqp->inmsg.data.srvcap.capstr);
			break;
		case PCK_AUTH:
			free(mqp->inmsg.data.auth.username);
			free(mqp->inmsg.data.auth.password);
			break;
		case PCK_SENDTOQUEUE:
			free(mqp->inmsg.data.stream.queue);
			free(mqp->inmsg.data.stream.topic);
			free(mqp->inmsg.data.stream.data);
			break;
		case PCK_JOINQUEUE:
			free(mqp->inmsg.data.joinqueue.queue);
			free(mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_QUEUEINFO:
			free(mqp->inmsg.data.joinqueue.queue);
			free(mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_MSGFROMQUEUE:
			free(mqp->inmsg.data.sendmsg.queue);
			free(mqp->inmsg.data.sendmsg.topic);
			free(mqp->inmsg.data.sendmsg.data);
			free(mqp->inmsg.data.sendmsg.from);
			break;
		default:
			if (mqplib->logger)
				mqplib->logger ("Invalid MsgType Recieved");
	}
	/* finished processing the message. grab what buffer we consumed and return */
	return usedbuf;
}

