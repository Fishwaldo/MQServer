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
** $Id$
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "client.h"
#include "list.h"
#include "dns.h"
#include "packet.h"

list_t *clientlist;

int init_client_list() {

	clientlist = list_create(-1);			
	if (!clientlist) {
		nlog(LOG_WARNING, LOG_CORE, "Warning, Client List create Failed");
		return NS_FAILURE;
	} else {
		return NS_SUCCESS;
	}
}


mqclient *new_client(int fd) {
	mqclient *mqc;
	lnode_t *node;
	
	if (list_isfull(clientlist)) {
		nlog(LOG_WARNING, LOG_CORE, "Client List Full. Bye Bye");
		do_exit(NS_EXIT_ERROR, "Client List Full");
	}
	
	mqc = malloc(sizeof(mqclient));
	bzero(mqc, sizeof(mqclient));
	mqc->type = MQC_TYPE_NONE;
	mqc->fd = fd;
	node = lnode_create(mqc);
	mqc->node = node;
	list_append(clientlist, node);
	nlog(LOG_DEBUG3, LOG_CORE, "New Client on Fd %d", fd);
#ifdef DEBUG
	if (!list_verify(clientlist)) {
		nlog(LOG_WARNING, LOG_CORE, "Client Verify after insert failed");
	}
#endif	
	return mqc;
}

static int list_find_fd(const void *key1, const void *key2) {
	const mqclient *mqc1 = key1;
	return ((int)key2 - mqc1->fd);
}

mqclient *find_client_from_fd(int fd) {
	lnode_t *node;
	
	node = list_find(clientlist, (void *)fd, list_find_fd);
	if (!node) {
		nlog(LOG_DEBUG1, LOG_CORE, "Can't Find Client for fd %d", fd);
		return NULL;
	}
	return lnode_get(node);
}

void del_client(mqclient *mqc) {
	nlog(LOG_DEBUG1, LOG_CORE, "Deleting Client %s (%d)", mqc->user, mqc->fd);
	list_delete(clientlist, mqc->node);
	lnode_destroy(mqc->node);
	if (mqc->bufferlen > 0) {
		free(mqc->buffer);
	}
	free(mqc);
}

void got_reverse_lookup_answer(void *data, adns_answer * a) {
	mqclient *mqc = data;
	int len, ri;
	char *show;
	if (a) {
		adns_rr_info(a->type, 0, 0, &len, 0, 0);
		if (a->nrrs > 0) {
			ri = adns_rr_info(a->type, 0, 0, 0, a->rrs.bytes, &show);
			if (!ri) {
				nlog(LOG_DEBUG2, LOG_CORE, "DNS for Host %s resolves to %s", mqc->host, show);
				strncpy(mqc->host, show, MAXHOST);
			} else {
				nlog(LOG_WARNING, LOG_CORE, "Dns Error: %s", adns_strerror(ri));
			}
			free(show);
		} else {
			nlog(LOG_DEBUG2, LOG_CORE, "DNS for IP %s Does not resolve", mqc->host);
		}
	}		
	MQC_CLEAR_STAT_DNSLOOKUP(mqc);
	/* XXX check bans? */


	/* ok, create a new client */
	if (MQC_IS_TYPE_CLIENT(mqc)) 
		mqc->pck = pck_new_conn((void *)mqc, PCK_IS_CLIENT);
	
	/* if there is data in the buffer, see if we can parse it already */
	while (mqc->offset >= PCK_MIN_PACK_SIZE) {
		len = pck_parse_packet(mqc->pck, mqc->buffer, mqc->offset);
		buffer_del(mqc, len);
		printf("dns processed %d bytes, %d left\n", len, mqc->offset);
	}

}

void do_reverse_lookup(mqclient *mqc) {
	/* at this stage, what ever is in host, will be a plain ip address */
	dns_lookup(mqc->host, adns_r_ptr, got_reverse_lookup_answer, (void *)mqc);
	MQC_SET_STAT_DNSLOOKUP(mqc);	
}

