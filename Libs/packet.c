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
#include "xds.h"

static int compare_mid(const void *key1, const void *key2) {
	mqpacket *mqpck = (void *)key1;
	unsigned long mid = (int)key2;
	
	if (mqpck->MID == mid) {
		return 0;
	} else {
		return -1;
	}
}

lnode_t *pck_find_mid_node(unsigned long MID, list_t *queue) {
	return (list_find(queue, (void *)MID, compare_mid));
}
	
void pck_create_mqpacket(mqpacket *mqpck, int type, xds_mode_t direction) {
	int i;
	mqpck = malloc(sizeof(mqpacket));
	if (xds_init(&mqpck->xds, direction) != XDS_OK) {
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
				}
				i++;
			}
			break;
		case ENG_TYPE_XML:
			while (i < NUMENGINES-1) {
				if (xds_register(mqpck->xds, dec_xml_engines[i].myname, dec_xml_engines[i].ptr, NULL) != XDS_OK) {
					if (mqpconfig.logger) 
						mqpconfig.logger("xds_register failed for %s", dec_xml_engines[i].myname);
				}
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

void pck_destroy_mqpacket(mqpacket *mqpck, mqprotocol *mqp) {
	lnode_t *node;
	
	if (mqp) {
		node = pck_find_mid_node(mqpck->MID, mqp->inpack);
		if (node) {
#ifdef DEBUG	
			if (mqpconfig.logger)
				mqpconfig.logger("Destroy Packet MessageID %lu", mqpck->MID);
#endif			
			list_delete(mqp->inpack, node);
			lnode_destroy(node);
		} 
	}
	free(mqpck->data);
	xds_destroy(mqpck->xds);
	free(mqpck);
	
}
