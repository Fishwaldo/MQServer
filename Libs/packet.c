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

#define PCK_MIN_PACK_SIZE 1
int pck_parse_packet(mqprotocol *mqp, u_char *buffer, unsigned long buflen) {
	mqpacket *mqpck;
	lnode_t *node;
	int i;
	u_short shorty;
	u_long longy;
	u_char hehe;
	u_char mypack[BUFSIZE];
	u_char *outbuf;
	
	int taken;
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
		if (mqpconfig.logger) 
			mqpconfig.logger("Got Packet Decode: mid %lu msgtype %d Version %d len %lu flags %lu crc %lu \n", mqpck->MID, mqpck->MSGTYPE, mqpck->VERSION, mqpck->LEN, mqpck->flags, mqpck->crc);
		mqpck->data = malloc(mqpck->LEN);
		strncpy(mqpck->data, outbuf, mqpck->LEN);
	}
}

void pck_send_err(mqprotocol *mqp, int err, const char *msg) {

	if (mqpconfig.logger) 
		mqpconfig.logger("pck_send_error: %s", msg);

}