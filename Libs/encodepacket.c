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


int sqlite_encode_binary (const unsigned char *in, int n, unsigned char *out);

mqpacket *
pck_new_packet (int msgtype, unsigned long flags)
{
	mqpacket *mqpck;

	mqpck = pck_create_mqpacket (ENG_TYPE_XDR, XDS_ENCODE);
	/* MID is null till we send, this way we can reuse this packet def */
	mqpck->MID = -1;
	mqpck->MSGTYPE = msgtype;
	mqpck->flags = flags;
	mqpck->dataoffset = 0;
	mqpck->data = NULL;
#ifdef DEBUG
	if (mqpconfig.logger)
		mqpconfig.logger ("Created a New Packet with MID %lu", mqpck->MID);
#endif
	return mqpck;
}

int
pck_set_packet_data (mqpacket * mqpck, void *data, size_t len)
{
	/* if its empty, malloc it */
	/* size is taken from encode.c sqlite_encode_binary description */
#if 0
	mqpck->data = malloc (2 + (257 * len) / 254);
	mqpck->dataoffset = sqlite_encode_binary (data, len, mqpck->data);
	printf ("Encode Size %lu Allocated %d\n", mqpck->dataoffset, 2 + (257 * len) / 254);
#endif
	return NS_SUCCESS;
}

unsigned long
pck_commit_data (mqprotocol * mqp, mqpacket * mqpck)
{
	lnode_t *node;
	uLong crc;
	int rc;

	if (list_isfull (mqp->outpack)) {
		if (mqpconfig.logger)
			mqpconfig.logger ("OutBuffer is Full.");
		return NS_FAILURE;
	}
	/* we are always at version one atm */
	mqpck->VERSION = 1;
	/* increment the mid */
	mqpck->MID = mqp->nxtoutmid++;

	if (xds_encode (mqpck->xds, "mqpheader", &mqpck) != XDS_OK) {
		if (mqpconfig.logger)
			mqpconfig.logger ("OutBuffer is Full.");
		pck_destroy_mqpacket (mqpck, NULL);
		return -1;
	}
	if (xds_getbuffer (mqpck->xds, XDS_GIFT, (void **) &mqpck->data, mqpck->dataoffset) != XDS_OK) {
		if (mqpconfig.logger)
			mqpconfig.logger ("OutBuffer is Full.");
		pck_destroy_mqpacket (mqpck, NULL);
		return -1;
	}

	node = lnode_create (mqpck);
	list_append (mqp->outpack, node);

	return mqpck->MID;
}
