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
pck_parse_packet (mqprotocol * mqp, u_char * buffer, unsigned long buflen)
{
	mqpacket *mqpck;
	u_char *buf2, *mybuf;
	int rc, len;

	if (mqp->wtforinpack == 1) {
		if (!strchr(buffer, '\0')) {
			if (mqpconfig.logger) 
				mqpconfig.logger ("Buffer isn't terminated\n");
		}
		mybuf = malloc(buflen);
		len = sqlite_decode_binary(buffer, mybuf);
		printf("decode %d\n", len);
		/* XXX XDR engine for now */
		mqpck = pck_create_mqpacket (ENG_TYPE_XDR, XDS_DECODE);
		if (mqpck == NULL) {
			if (mqpconfig.logger)
				mqpconfig.logger("create packet failed");
			/* XXX drop client ? */
			free(mybuf);
			return NS_FAILURE;
		}
		if (xds_setbuffer (mqpck->xds, XDS_LOAN, mybuf, len) != XDS_OK) {
			if (mqpconfig.logger)
				mqpconfig.logger ("XDS setbuffer Failed");
			pck_destroy_mqpacket (mqpck, NULL);
			/* XXX drop client ? */
			free(mybuf);
			return NS_FAILURE;
		}
		/* lets use a switch here to handle it intelegently */
		rc = xds_decode (mqpck->xds, "mqpheader", &mqpck);
		free(mybuf);
		switch (rc) {
		case XDS_OK:
			printf("breaking here\n");
			break;
		case XDS_ERR_UNDERFLOW:
			if (mqpconfig.logger)
				mqpconfig.logger ("XDS Decode of Header Failed. Buffer Underflow");
			pck_destroy_mqpacket (mqpck, NULL);
			/* don't consume any buffer */
			return NS_SUCCESS;
		default:
			if (mqpconfig.logger)
				mqpconfig.logger ("XDS Decode of Header Failed: %d", rc);
			pck_destroy_mqpacket (mqpck, NULL);
			/* drop client */
			return NS_FAILURE;
		}
		if (mqpconfig.logger)
			mqpconfig.logger ("Got Packet Decode: mid %lu msgtype %d Version %d flags %lu ", mqpck->MID, mqpck->MSGTYPE, mqpck->VERSION, mqpck->flags);

		/* check the version number */
		if (mqpck->VERSION != 1) {
			if (mqpconfig.logger)
				mqpconfig.logger ("Invalid Protocol Version recieved");
			pck_destroy_mqpacket (mqpck, NULL);
			/* XXX drop client ? */
			return NS_FAILURE;
		}
		/* ok, lets handle this message based on the message type */
		switch (mqpck->MSGTYPE) {


		default:
			if (mqpconfig.logger)
				mqpconfig.logger ("Invalid MsgType Recieved");
//			pck_destroy_mqpacket (mqpck, NULL);
			/* XXX drop client */
			return NS_FAILURE;
		}
		/* finished processing the message. grab what buffer we consumed and return */
//		rc = xds_get_usedbuffer (mqpck->xds);
//		pck_destroy_mqpacket (mqpck, NULL);
		return len;
	}

	/* nothing done on the buffer, so return 0 */
	return 0;
}

void
pck_send_err (mqprotocol * mqp, int err, const char *msg)
{

	if (mqpconfig.logger)
		mqpconfig.logger ("pck_send_error: %s", msg);

}
