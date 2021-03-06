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

/* needed for crc32 function */
#include <zlib.h>

#include "libmq.h"
#include "list.h"
#include "packet.h"
#include "xds.h"


/* this is called from the socket recv code. */
int
pck_parse_packet (mqp *mqplib, mqpacket * mqp)
{
	int rc, usedbuf;

	
	/* XXX XDR engine for now */
#if 0
	printf("%d\n", mqp->inbuf->off);
#endif
	if (xds_setbuffer (mqp->xdsin, XDS_LOAN, mqp->inbuf->buffer, mqp->inbuf->off) != XDS_OK) {
		MQLOG(mqplib, MQLOG_WARNING, "XDS setbuffer Failed");
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
			MQLOG(mqplib, MQLOG_INFO, "XDS Decode of Header Failed. Buffer Underflow");
			/* don't consume any buffer */
			return -2;
		default:
#ifdef PACKDEBUG
			TRACE(mqplib, MQDBG4, "PackDebug: %s", mqp->inbuf->buffer);
#endif
			MQLOG(mqplib, MQLOG_WARNING,"XDS Decode of Header Failed: %d", rc);
			print_decode(mqp, 1);
			/* drop client */
			return NS_FAILURE;
	}
	TRACE(mqplib, MQDBG1, "Got Packet Decode: mid %d msgtype %d Version %d flags %d ", mqp->inmsg.MID, mqp->inmsg.MSGTYPE, mqp->inmsg.VERSION, mqp->inmsg.flags);
		/* check the version number */
	if (mqp->inmsg.VERSION != 1) {
		MQLOG(mqplib, MQLOG_WARNING, "Invalid Protocol Version recieved");
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
			MQLOG(mqplib, MQLOG_WARNING,"Invalid MsgType Recieved");
			/* XXX drop client */
			return NS_FAILURE;
	}
	switch (rc) {
		case XDS_OK:
			usedbuf = xds_get_usedbuffer (mqp->xdsin);
			break;
		case XDS_ERR_UNDERFLOW:
			/* don't consume any buffer */
			return -2;
		default:
			MQLOG(mqplib, MQLOG_WARNING, "XDS Decode of Data Failed: %d", rc);
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
				MQLOG(mqplib, MQLOG_INFO, "XDS Decode of Footer Failed. Buffer Underflow");
				/* don't consume any buffer */
				return -2;
			default:
				MQLOG(mqplib, MQLOG_WARNING, "XDS Decode of Footer Failed: %d", rc);
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
			(mqlib_free)(mqp->inmsg.data.string);
			break;
		case PCK_SRVCAP:
		case PCK_CLNTCAP:
			(mqlib_free)(mqp->inmsg.data.srvcap.capstr);
			break;
		case PCK_AUTH:
			(mqlib_free)(mqp->inmsg.data.auth.username);
			(mqlib_free)(mqp->inmsg.data.auth.password);
			break;
		case PCK_SENDTOQUEUE:
			(mqlib_free)(mqp->inmsg.data.stream.queue);
			(mqlib_free)(mqp->inmsg.data.stream.topic);
			(mqlib_free)(mqp->inmsg.data.stream.data);
			break;
		case PCK_JOINQUEUE:
			(mqlib_free)(mqp->inmsg.data.joinqueue.queue);
			(mqlib_free)(mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_QUEUEINFO:
			(mqlib_free)(mqp->inmsg.data.joinqueue.queue);
			(mqlib_free)(mqp->inmsg.data.joinqueue.filter);
			break;
		case PCK_MSGFROMQUEUE:
			(mqlib_free)(mqp->inmsg.data.sendmsg.queue);
			(mqlib_free)(mqp->inmsg.data.sendmsg.topic);
			(mqlib_free)(mqp->inmsg.data.sendmsg.data);
			(mqlib_free)(mqp->inmsg.data.sendmsg.from);
			break;
		default:
			MQLOG(mqplib, MQLOG_WARNING, "Invalid MsgType Recieved");
	}
	/* finished processing the message. grab what buffer we consumed and return */
	return usedbuf;
}

