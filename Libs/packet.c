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
#include "getchar.h"

void pck_send_err(mqprotocol *mqp, int err, const char *msg);


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

#define PCK_MIN_PACK_SIZE 84
int pck_parse_packet(mqprotocol *mqp, u_char *buffer, unsigned long buflen) {
	mqpacket *mqpck;
	u_char *outbuf;
	int gotdatasize;
	lnode_t *node;

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
		gotdatasize = strlen(outbuf);
#ifdef DEBUG
		if (mqpconfig.logger)
			mqpconfig.logger("Header Size: %d Data Size %lu Real Data Size: %d", (*outbuf - *buffer), mqpck->LEN, gotdatasize);
#endif
		if (mqpconfig.logger) 
			mqpconfig.logger("Got Packet Decode: mid %lu msgtype %d Version %d len %lu flags %lu crc %lu", mqpck->MID, mqpck->MSGTYPE, mqpck->VERSION, mqpck->LEN, mqpck->flags, mqpck->crc);

		mqpck->data = malloc(mqpck->LEN);
		bzero(mqpck->data, mqpck->LEN);
		strncat(mqpck->data, outbuf, mqpck->LEN);

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
			/* XXX ok, process this packet */
		} else {
#ifdef DEBUG		
			if (mqpconfig.logger)
				mqpconfig.logger("Got More data than we expected. Throwing Away MessageID %lu", mqpck->MID);
#endif	
			/* XXX TODO Drop Client?*/
		}			

		return PCK_MIN_PACK_SIZE + mqpck->LEN;
	} else {
		/* XXX got additional data for previous message */
	}
	/* nothing done on the buffer, so return 0 */
	return 0;
}

void pck_send_err(mqprotocol *mqp, int err, const char *msg) {

	if (mqpconfig.logger) 
		mqpconfig.logger("pck_send_error: %s", msg);

}