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

#define SRVCAP1 2
#define SRVCAP2 3

#define CLNTCAP1 2
#define CLNTCAP2 3

unsigned long pck_commit_data (mqp * mqplib, mqpacket * mqpck);



int pck_prepare(mqp *mqplib, mqpacket *mqp, int type) {
	int rc;
	if ((mqp) && (mqp->outmsg.MID == -1)) {
		mqp->outmsg.MID = mqp->nxtoutmid++;
		mqp->outmsg.VERSION = 1;
		mqp->outmsg.MSGTYPE = type;
		mqp->outmsg.flags = 9;
		if (mqp->wiretype == ENG_TYPE_XML) {
			rc = xds_encode (mqp->xdsout, "xmlstart mqpheader", mqp);
		} else {
			rc = xds_encode (mqp->xdsout, "mqpheader", mqp);
		}
		if (rc != XDS_OK) {
			if (mqplib->logger)
				mqplib->logger ("xds encode header failed %d.", rc);
			return NS_FAILURE;
		}
		return NS_SUCCESS;
	} else {
		if (mqplib->logger) {
			mqplib->logger("Outbound Message Not clear %d", mqp->outmsg.MID);
		}
		return NS_FAILURE;
	}
}

int pck_remove(mqp *mqplib, mqpacket *mqp) {
	mqp->outmsg.MID = -1;
	return NS_SUCCESS;
}


unsigned long pck_send_ack(mqp *mqplib, mqpacket *mqp, int MID) {
	int rc;
	
	if (pck_prepare(mqplib, mqp, PCK_ACK) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqp->xdsout, PCK_ACK_FMT, MID);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode ack failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqp));
}

unsigned long pck_send_srvcap(mqp *mqplib, mqpacket *mqp) {
	int rc;
	char srvcap[BUFSIZE];
	
	
	if (pck_prepare(mqplib, mqp, PCK_SRVCAP) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	
	snprintf(srvcap, BUFSIZE, "STD");
#ifdef DEBUG	
	strncat(srvcap, ":DBG", BUFSIZE);
#endif	
	rc = xds_encode (mqp->xdsout, PCK_SRVCAP_FMT, SRVCAP1, SRVCAP2, srvcap);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode srvcap failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqp));
}

unsigned long pck_send_clntcap(mqp *mqplib, mqpacket *mqp) {
	int rc;
	char clntcap[BUFSIZE];
	
	
	if (pck_prepare(mqplib, mqp, PCK_CLNTCAP) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	
	snprintf(clntcap, BUFSIZE, "STD");
#ifdef DEBUG	
	strncat(clntcap, ":DBG", BUFSIZE);
#endif	
	rc = xds_encode (mqp->xdsout, PCK_CLNTCAP_FMT, CLNTCAP1, CLNTCAP2, clntcap);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode clntcap failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqp));
}

unsigned long pck_send_error(mqp *mqplib, mqpacket *mqp, char *fmt, ...) {
	int rc;
	va_list ap;
	char log_buf[BUFSIZE];
		
	if (pck_prepare(mqplib, mqp, PCK_ERROR) != NS_SUCCESS) {
		return NS_FAILURE;
	}

	va_start(ap, fmt);
	vsnprintf(log_buf, BUFSIZE, fmt, ap);
	va_end(ap);

	rc = xds_encode (mqp->xdsout, PCK_ERROR_FMT, log_buf);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode error failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqp));
}

unsigned long pck_send_auth(mqp *mqplib, mqpacket *mqp, char *username, char *password) {
	int rc;
		
	if (pck_prepare(mqplib, mqp, PCK_AUTH) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqp->xdsout, PCK_AUTH_FMT, username, password);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode auth failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqp));
}


unsigned long
pck_commit_data (mqp * mqplib, mqpacket * mqpck)
{
	int rc;
	char *buffer;
	size_t buflen;

	if (mqpck->wiretype == ENG_TYPE_XML) {
		rc = xds_encode (mqpck->xdsout, "xmlstop");

		if (rc != XDS_OK) {
			if (mqplib->logger)
				mqplib->logger ("xds encode xmlstop failed %d.", rc);
			return NS_FAILURE;
		}
	}
	rc = xds_getbuffer (mqpck->xdsout, XDS_GIFT, (void **) &buffer, &buflen) ;
	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("OutBuffer is Full. %d", rc);
		return NS_FAILURE;
	}
	mqbuffer_add(mqpck->outbuf, buffer, buflen);
	write_fd(mqplib, mqpck);
	rc = mqpck->outmsg.MID;
	pck_remove(mqplib, mqpck);
	return rc;
}


unsigned long 
pck_send_message_struct(mqp *mqplib, mqpacket *mqpck, structentry *mystruct, int cols, void *data, char *destination, char *topic) {
	int rc, i, myint;
	void *mydata;
	void *buffer;
	size_t bufferlen;


	for (i = 0; i < cols; i++) {
		switch(mystruct[i].type) {
			case STR_PSTR:
				mydata = (mystruct[i].readcb)(data, &rc);
				rc = xds_encode (mqpck->xdsout, "string", mydata);
				if (rc != XDS_OK) {
					if (mqplib->logger)
						mqplib->logger ("xds encode header failed %d.", rc);
					return NS_FAILURE;
				}
				
/* 				free(mydata); */
				break;
			case STR_INT:
				mydata = data + mystruct[i].offset;
				myint = *(int *)mydata;
				rc = xds_encode (mqpck->xdsout, "int32", myint);
				if (rc != XDS_OK) {
					if (mqplib->logger)
						mqplib->logger ("xds encode header failed %d.", rc);
					return NS_FAILURE;
				}
				break;
			case STR_STR:
				mydata = data + mystruct[i].offset;
				if (strlen(mydata) > mystruct[i].size) {
					if (mqplib->logger) 
						mqplib->logger ("String In Column %d is too long (%d > %d). Not Encoding", i, strlen(mydata), mystruct[i].size);
					break;
				}
				rc = xds_encode (mqpck->xdsout, "string", (char *)mydata);
				if (rc != XDS_OK) {
					if (mqplib->logger)
						mqplib->logger ("xds encode header failed %d.", rc);
					return NS_FAILURE;
				}
				break;
		}
	}
	rc = xds_getbuffer (mqpck->xdsout, XDS_GIFT, (void **) &buffer, &bufferlen) ;
	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("OutBuffer is Full. %d", rc);
		return NS_FAILURE;
	}
	if (pck_prepare(mqplib, mqpck, PCK_SENDTOQUEUE) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqpck->xdsout, PCK_SENDTOQUEUE_FMT, destination, bufferlen, topic, buffer, bufferlen);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode message failed %d.", rc);
		return NS_FAILURE;
	}
	free(buffer);
	return (pck_commit_data(mqplib, mqpck));

}

unsigned long
pck_send_joinqueue(mqp *mqplib, mqpacket *mqpck, char *queue, int flags, char *filter) {
	int rc;
		
	if (pck_prepare(mqplib, mqpck, PCK_JOINQUEUE) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqpck->xdsout, PCK_JOINQUEUE_FMT, queue, flags, filter);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode joinqueue failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqpck));
}

unsigned long 
pck_send_queueinfo(mqp *mqplib, mqpacket *mqpck, char *queue, char *info, int flags) {
	int rc;
		
	if (pck_prepare(mqplib, mqpck, PCK_QUEUEINFO) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqpck->xdsout, PCK_QUEUEINFO_FMT, queue, flags, info);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode queueinfo failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqpck));
}

unsigned long
pck_send_queue_mes(mqp *mqplib, mqpacket *mqpck, char *queue, char *topic, void *data, size_t len, unsigned long messid, long timestamp, char *from) {
	int rc;
		
	if (pck_prepare(mqplib, mqpck, PCK_MSGFROMQUEUE) != NS_SUCCESS) {
		return NS_FAILURE;
	}
	rc = xds_encode (mqpck->xdsout, PCK_MSGFROMQUEUE_FMT, queue, topic, data, len, messid, timestamp, from);

	if (rc != XDS_OK) {
		if (mqplib->logger)
			mqplib->logger ("xds encode msgfromqueue failed %d.", rc);
		return NS_FAILURE;
	}
	
	return (pck_commit_data(mqplib, mqpck));
}
