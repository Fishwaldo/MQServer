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


/* this is called from the socket recv code. */
int pck_parse_packet(mqprotocol *mqp, u_char *buffer, unsigned long buflen) {
	mqpacket *mqpck;
	u_char *outbuf;
	int gotdatasize;
	int rc;
	lnode_t *node;

	if (mqp->wtforinpack == 1) {
		/* XXX XDR engine for now */
		pck_create_mqpacket(mqpck, ENG_TYPE_XDR, XDS_DECODE);
		if (mqpck = NULL) {
			/* XXX drop client ? */
			return -1;
		}
		if (xds_setbuffer(mqpck->xds, XDS_LOAN, buffer, buflen) != XDS_OK) {
			if (mqpconfig.logger) 
				mqpconfig.logger("XDS Decode of Header Failed");
			pck_destroy_mqpacket(mqpck, NULL);
			/* XXX drop client ? */
			return -1;
		}			
		/* lets use a switch here to handle it intelegently */
		rc = xds_decode(mqpck->xds, "mqpheader", &mqpck);
		switch (rc) {
			case XDS_OK:
				break;
			case XDS_ERR_UNDERFLOW:
				if (mqpconfig.logger) 
					mqpconfig.logger("XDS Decode of Header Failed. Buffer Underflow");
				pck_destroy_mqpacket(mqpck, NULL);
				/* don't consume any buffer */
				return 0; 
			default:
				if (mqpconfig.logger) 
					mqpconfig.logger("XDS Decode of Header Failed: %d", rc);
				pck_destroy_mqpacket(mqpck, NULL);
				/* drop client */
				return -1; 
		}		
		if (mqpconfig.logger) 
			mqpconfig.logger("Got Packet Decode: mid %lu msgtype %d Version %d flags %lu ", mqpck->MID, mqpck->MSGTYPE, mqpck->VERSION, mqpck->flags);
		
		/* check the version number */
		if (mqpck->VERSION != 1) {
			if (mqpconfig.logger) 
				mqpconfig.logger("Invalid Protocol Version recieved");
			pck_destroy_mqpacket(mqpck, NULL);
			/* XXX drop client ? */
			return -1;
		}
		/* ok, lets handle this message based on the message type */	
		switch (mqpck->MSGTYPE) {
		
		
			default:
				if (mqpconfig.logger) 
					mqpconfig.logger("Invalid MsgType Recieved");
				pck_destroy_mqpacket(mqpck, NULL);
				/* XXX drop client */
				return -1;
		}
		/* finished processing the message. grab what buffer we consumed and return */
									
		rc = xds_get_usedbuffer(mqpck->xds);	
		pck_destroy_mqpacket(mqpck, NULL);
		return rc;
	}

	/* nothing done on the buffer, so return 0 */
	return 0;
}

void pck_send_err(mqprotocol *mqp, int err, const char *msg) {

	if (mqpconfig.logger) 
		mqpconfig.logger("pck_send_error: %s", msg);

}
