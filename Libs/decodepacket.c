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
#include "getchar.h"
#include "xds.h"

static void pck_send_err(mqprotocol *mqp, int err, const char *msg);
static int pck_check_packet_crc(mqpacket *, mqprotocol *);
static void pck_destroy_mqpacket(mqpacket *mqpck, mqprotocol *mqp);

void pck_init() {
}

void pck_set_logger(logfunc *logger) {
	mqpconfig.logger = logger;
}

mqprotocol *pck_new_conn(void *cbarg, int type) {
	mqprotocol *mqp;
	
	if (type > 0 && type < 4) {
		mqp = malloc(sizeof(mqprotocol));
		bzero(mqp, sizeof(mqprotocol));
		mqp->inpack = list_create(INPACK_MAX);
		mqp->outpack = list_create(OUTPACK_MAX);
		mqp->cbarg = cbarg;
		mqp->servorclnt = type;
		/* we are waiting for a new packet */
		mqp->wtforinpack = 1;
		if (mqpconfig.logger)
			mqpconfig.logger("New Protocol Struct created");
		return mqp;
	} else {
		if (mqpconfig.logger) 
			mqpconfig.logger("Invalid Connection Type %d", type);
	}					
	return NULL;
}
static int compare_mid(const void *key1, const void *key2) {
	mqpacket *mqpck = (void *)key1;
	unsigned long mid = (int)key2;
	
	if (mqpck->MID == mid) {
		return 0;
	} else {
		return -1;
	}
}

static lnode_t *pck_find_mid_node(unsigned long MID, list_t *queue) {
	return (list_find(queue, (void *)MID, compare_mid));
}
	

	
		
void pck_create_mqpacket(mqpacket *mqpck, int type) {
	int i;
	mqpck = malloc(sizeof(mqpacket));
	if (xds_init(&mqpck->xds, XDS_DECODE) != XDS_OK) {
		if (mqpconfig.logger)
			mqpconfig.logger("xds init failed: %s", strerrno(errno));
	}
	i = 0;
	switch (type) {
		case ENG_TYPE_XDR:
			while (i < NUMENGINES) {
				if (xds_register(mqpck->xds, dec_xdr_engines[i].myname, dec_xdr_engines[i].ptr, NULL) != XDS_OK) {
				if (mqpconfig.logger) 
					mqpconfig.logger("xds_register failed for %s", dec_xdr_engines[i].myname);
				i++;
			}
			break;
		case ENG_TYPE_XML:
			while (i < NUMENGINES) {
				if (xds_register(mqpck->xds, dec_xml_engines[i].myname, dec_xml_engines[i].ptr, NULL) != XDS_OK) {
				if (mqpconfig.logger) 
					mqpconfig.logger("xds_register failed for %s", dec_xml_engines[i].myname);
				i++;
			}
			break;
		default:
			if (mqpconfig.logger)
				mqpconfig.logger("invalid pck_create_mqpacket type %d", type);
			xds_destroy(mqpck->xds);
			free(mqpck);
			return;
	}
}

/* this is called from the socket recv code. */
int pck_parse_packet(mqprotocol *mqp, u_char *buffer, unsigned long buflen) {
	mqpacket *mqpck;
	u_char *outbuf;
	int gotdatasize;
	lnode_t *node;

	if (mqp->wtforinpack == 1) {
		/* XXX XDR engine for now */
		pck_create_mqpacket(mqpck, ENG_TYPE_XDR);
		if (mqpck = NULL) {
			/* XXX drop client ? */
			return 0;
		}
	
	
	
	
	
	}

#if 0

	/* first thing we do is decide if the buffer holds the minium packet size required.*/
	if (buflen < PCK_MIN_PACK_SIZE) {
		return 0;
	}
	/* ok, if we are waiting for a new packet, lets decode the header */
	if (mqp->wtforinpack == 1) {
		/* if our buffer is full, signal it. */
		if (list_isfull(mqp->inpack)) {
			pck_send_err(mqp, PCK_ERR_BUFFULL, "Inbound Buffer is full");
			return -1;
		}
		mqpck = malloc(sizeof(mqpacket));
		/* dont add it to the list till we do a proper decode on it. */
		outbuf = buffer;
		GETLONG(mqpck->MID, outbuf);
		GETSHORT(mqpck->MSGTYPE, outbuf);
		GETSHORT(mqpck->VERSION, outbuf);
		GETLONG(mqpck->LEN, outbuf);
		GETLONG(mqpck->flags, outbuf);
		GETLONG(mqpck->crc, outbuf);
		/* even though the rest of this is data, it might be additional flags. 
		 * we encode this data, so its a C "string", ie, no Nulls. so we will only 
		 * decode it once we get the entire string. */
		gotdatasize = strlen(outbuf);
#ifdef DEBUG
		if (mqpconfig.logger)
			mqpconfig.logger("Header Size: %d Data Size %lu Real Data Size: %d", (*outbuf - *buffer), mqpck->LEN, gotdatasize);
#endif
		if (mqpconfig.logger) 
			mqpconfig.logger("Got Packet Decode: mid %lu msgtype %d Version %d len %lu flags %lu crc %lu", mqpck->MID, mqpck->MSGTYPE, mqpck->VERSION, mqpck->LEN, mqpck->flags, mqpck->crc);

		mqpck->data = malloc(mqpck->LEN);
		bzero(mqpck->data, mqpck->LEN);
		/* seee above explaination about gotdatasize to see why treating this like a string is ok*/
		strncpy(mqpck->data, outbuf, mqpck->LEN);

		/* ok, seems fine so far, add it to the queue */
		node = lnode_create(mqpck);
		list_append(mqp->inpack, node);
	
		/* check if we got the entire message yet */
		if (mqpck->LEN > gotdatasize) {
			/* we are still waiting for more data */
			mqp->wtforinpack = 0;
#ifdef DEBUG		
			if (mqpconfig.logger)
				mqpconfig.logger("Wating for More Data (%lu) for MessageID %lu", (mqpck->LEN - gotdatasize), mqpck->MID);
#endif	
		} else if (mqpck->LEN == gotdatasize) {
#ifdef DEBUG		
			if (mqpconfig.logger)
				mqpconfig.logger("Got All Our Data. Processing MessageID %lu", mqpck->MID);
#endif	
			if (pck_check_packet_crc(mqpck, mqp) == NS_SUCCESS) {
				/* XXX ok, process this packet */
				/* finished. ok, process next pack */
			}
			mqp->wtforinpack = 1;
		} else {
			/* XXX TODO Drop Client?*/
		}			

		return PCK_MIN_PACK_SIZE + gotdatasize;
	} else {
		/* additional data is encoded as a string */
		node = list_last(mqp->inpack);
		/* ehhh, inpack node is empty. Something is screwy */
		if (node == NULL) {
			if (mqpconfig.logger)
				mqpconfig.logger("Got lastnode is empty");
			return -1;
		}
		mqpck = lnode_get(node);

		/* this is additional data, and it *should* be terminted by a null, so strlen should work */
		gotdatasize = strlen(buffer);
		if ((mqpck->LEN - strlen(mqpck->data)) == gotdatasize) {
			/* this is the final pack, tack it on the end, and then pass it off for processing */
			strncat(mqpck->data, buffer, mqpck->LEN);
			if (pck_check_packet_crc(mqpck, mqp) == NS_SUCCESS) {
				/* XXX process it. */
			}
			mqp->wtforinpack = 1;
			return gotdatasize;
		} else if ((mqpck->LEN - strlen(mqpck->data) - gotdatasize) < 0) {
			/* we got more data than we expected. Eeeek */
#ifdef DEBUG
			if (mqpconfig.logger)
				mqpconfig.logger("Got More data in buffer overflow than we expected. Throwing away MessageID %lu", mqpck->MID);
#endif			
	
			/* XXX TODO Drop Client? */
		} else {
			/* we must be waiting for more data still. Tack it on, and update. */
			strncat(mqpck->data, buffer, mqpck->LEN);
#ifdef DEBUG
			if (mqpconfig.logger)
				mqpconfig.logger("still waiting for more data in buffer overflowfor MessageID %lu", mqpck->MID);
#endif			
			
		}
	}
#endif
	/* nothing done on the buffer, so return 0 */
	return 0;
}

void pck_send_err(mqprotocol *mqp, int err, const char *msg) {

	if (mqpconfig.logger) 
		mqpconfig.logger("pck_send_error: %s", msg);

}

int pck_check_packet_crc(mqpacket *mqpck, mqprotocol *mqp) {
	uLong decodecrc;
	
	/* seed it */
	decodecrc = crc32(0L, Z_NULL, 0);
	
	/* now do it */
	decodecrc = crc32(decodecrc, mqpck->data, mqpck->LEN);
	if (decodecrc != mqpck->crc) {
		/* CRC Failed */
		pck_send_err(mqp, PCK_ERR_CRC, "Crc Failed for MessageID");
		pck_destroy_mqpacket(mqpck, mqp);
		return NS_FAILURE;
	}
	return NS_SUCCESS;
}


void pck_destroy_mqpacket(mqpacket *mqpck, mqprotocol *mqp) {
	lnode_t *node;
	
	node = pck_find_mid_node(mqpck->MID, mqp->inpack);
	if (node) {
#ifdef DEBUG
			if (mqpconfig.logger)
				mqpconfig.logger("Destroy Packet MessageID %lu", mqpck->MID);
#endif			
		list_delete(mqp->inpack, node);
		lnode_destroy(node);
		free(mqpck->data);
		free(mqpck);
	} 

}