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


int close_fd(int fd, mqprotocol *mqp);
int buffer_add(mqprotocol *mqp, void *data, size_t datlen);
void buffer_del(mqprotocol *mqp, size_t drainlen);

void pck_init() {
	connections = list_create(-1);
}

void pck_set_logger(logfunc *logger) {
	mqpconfig.logger = logger;
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

static int compare_fd(const void *key1, const void *key2) {
	mqprotocol *mqp = (void *)key1;
	unsigned long fd = (int)key2;
	
	if (mqp->sock == fd) {
		return 0;
	} else {
		return -1;
	}
}

lnode_t *pck_find_fd_node(unsigned long fd, list_t *queue) {
	return (list_find(queue, (void *)fd, compare_fd));
}


lnode_t *pck_find_mid_node(unsigned long MID, list_t *queue) {
	return (list_find(queue, (void *)MID, compare_mid));
}
	
void pck_create_mqpacket(mqpacket *mqpck, int type, xds_mode_t direction) {
	int i;
	mqpck = malloc(sizeof(mqpacket));
	if (xds_init(&mqpck->xds, direction) != XDS_OK) {
		if (mqpconfig.logger)
			mqpconfig.logger("xds init failed: %s", strerror(errno));
			free(mqpck);
			mqpck = NULL;
	}
	i = 0;
	switch (type) {
		case ENG_TYPE_XDR:
			while (i < NUMENGINES) {
				if (xds_register(mqpck->xds, dec_xdr_engines[i].myname, dec_xdr_engines[i].ptr, NULL) != XDS_OK) {
					if (mqpconfig.logger) 
						mqpconfig.logger("xds_register failed for %s", dec_xdr_engines[i].myname);
					xds_destroy(mqpck->xds);
					free(mqpck);
					mqpck = NULL;
				}
				i++;
			}
			break;
		case ENG_TYPE_XML:
			while (i < NUMENGINES-1) {
				if (xds_register(mqpck->xds, dec_xml_engines[i].myname, dec_xml_engines[i].ptr, NULL) != XDS_OK) {
					if (mqpconfig.logger) 
						mqpconfig.logger("xds_register failed for %s", dec_xml_engines[i].myname);
					xds_destroy(mqpck->xds);
					free(mqpck);
					mqpck = NULL;
				}
				i++;
			}
			break;
		default:
			if (mqpconfig.logger)
				mqpconfig.logger("invalid pck_create_mqpacket type %d", type);
			xds_destroy(mqpck->xds);
			free(mqpck);
			mqpck = NULL;
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


mqprotocol *pck_new_conn(void *cbarg, int type) {
	mqprotocol *mqp;
	lnode_t *node;
	
	if (type > 0 && type < 4) {
		mqp = malloc(sizeof(mqprotocol));
		bzero(mqp, sizeof(mqprotocol));
		mqp->inpack = list_create(-1);
		mqp->outpack = list_create(-1);
		mqp->cbarg = cbarg;
		mqp->sock = 0;
		mqp->pollfdopts = 0;
		mqp->servorclnt = type;
		/* we are waiting for a new packet */
		mqp->wtforinpack = 1;
		if (mqpconfig.logger)
			mqpconfig.logger("New Protocol Struct created");
		node = lnode_create(mqp);
		list_append(connections, node);
		return mqp;
	} else {
		if (mqpconfig.logger) 
			mqpconfig.logger("Invalid Connection Type %d", type);
	}					
	return NULL;
}

int pck_free_conn(mqprotocol *mqp) {
	lnode_t *node, *node2;
	mqprotocol *mqp2;
	mqpacket *mqpck;
	
	node = list_first(connections);
	while (node != NULL) {
		mqp2 = lnode_get(node);
		if (mqp2 == mqp) {
			/* its a match */
			list_delete(connections, node);
			lnode_destroy(node);
			node2 = list_first(mqp->inpack);
			while (node2 != NULL) {
				mqpck = lnode_get(node2);
				free(mqpck->data);
			        xds_destroy(mqpck->xds);
		                free(mqpck);
				node2 = list_next(mqp->inpack, node2);
			}
			node2 = list_first(mqp->outpack); 
			while (node2 != NULL) {
				mqpck = lnode_get(node2);
				free(mqpck->data);
			        xds_destroy(mqpck->xds);
		                free(mqpck);
				node2 = list_next(mqp->outpack, node2);
			}
			list_destroy_nodes(mqp->inpack);
			list_destroy_nodes(mqp->outpack);
			list_destroy(mqp->inpack);
			list_destroy(mqp->outpack);
			free(mqp);
			if (mqpconfig.logger) 
				mqpconfig.logger("Deleted Protocol Struct");
			return NS_SUCCESS;
		}
	}
	if (mqpconfig.logger) 
		mqpconfig.logger("Couldn't Find Protocol Struct for deletion");
	return NS_FAILURE;
}

int read_fd(int fd, void *arg) {
	void *buf[BUFSIZE];
	mqprotocol *mqp;
	int i;
	lnode_t *node;

	
	node = pck_find_fd_node(fd, connections);
	if (node) {
		mqp = lnode_get(node);
		bzero(buf, BUFSIZE);
		i = read(fd, buf, BUFSIZE);
		if (i < 1) {
			/* error */
			close_fd(fd, mqp);
			/* XXX close and clean up */
			return NS_FAILURE;
		} else {
			buffer_add(mqp, buf, i);
		}

		i = pck_parse_packet(mqp, mqp->buffer, mqp->offset);
		if (mqpconfig.logger) 
			mqpconfig.logger("processed %d bytes %d left", i, mqp->offset);
		if (i > 0) {
		 	buffer_del(mqp, i);
		} else if (i == -1) {
			close_fd(fd, mqp);
			return NS_FAILURE;
		}	
	} else {
		if (mqpconfig.logger)
			mqpconfig.logger("Can't find client for %d", fd);
		close_fd(fd, NULL);
		return NS_FAILURE;
	}
	return NS_SUCCESS;
}

int close_fd(int fd, mqprotocol *mqp) {
	if (mqp) {
		pck_free_conn(mqp);
	} else {
		if (mqpconfig.logger)
			mqpconfig.logger("Closing a FD without a connection?");
	}
	close(fd);
	return NS_SUCCESS;
}


int
buffer_add(mqprotocol *mqp, void *data, size_t datlen)
{
	size_t need = mqp->offset + datlen;
//	size_t oldoff = mqp->offset;

	if (mqp->bufferlen < need) {
		void *newbuf;
		int length = mqp->bufferlen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if ((newbuf = realloc(mqp->buffer, length)) == NULL)
			return (-1);
		mqp->buffer = newbuf;
		mqp->bufferlen = length;
	}

	memcpy(mqp->buffer + mqp->offset, data, datlen);
	mqp->offset += datlen;

#if 0
	if (datlen && mqp->cb != NULL)
		(*mqp->cb)(mqp, oldoff, mqp->off, mqp->cbarg);
#endif
	return (0);
}

void buffer_del(mqprotocol *mqp, size_t drainlen) {
	
	memmove(mqp->buffer, mqp->buffer + drainlen, mqp->offset-drainlen);
	mqp->offset = mqp->offset-drainlen;
}

int write_fd(mqprotocol *mqp) {
	int i;
	lnode_t *node;
	mqpacket *mqpacket;
	if (mqp->outbufferlen > 0) {
		i = write(mqp->sock, mqp->outbuffer, mqp->outbufferlen);
		if (i == mqp->outbufferlen) {
			free(mqp->outbuffer);
			mqp->outbufferlen = mqp->outoffset = 0;
		} else if (i > 0) {
			memmove(mqp->outbuffer, mqp->outbuffer + i, mqp->outoffset - i);
			mqp->outoffset = mqp->outoffset -i;
		} else {
			/* something went wrong sending the data */
			if (mqpconfig.logger)
				mqpconfig.logger("Error Sending on fd %d", mqp->sock);
			close_fd(mqp->sock, mqp);
			return NS_FAILURE;
		}			
	}
	if (list_count(mqp->outpack) > 0) {
		node = list_first(mqp->outpack);
		mqpacket = lnode_get(node);
		mqp->outbuffer = malloc(2 +(257*mqpacket->dataoffset)/254);
		mqp->outbufferlen = sqlite_encode_binary(mqpacket->data, mqpacket->dataoffset, mqp->outbuffer);
		mqp->outoffset = mqp->outbufferlen;
		if (mqp->outbufferlen > 0) {
			list_delete(mqp->outpack, node);
			lnode_destroy(node);
			i = write(mqp->sock, mqp->outbuffer, mqp->outbufferlen);
			if (i == mqp->outbufferlen) {
				free(mqp->outbuffer);
				mqp->bufferlen = mqp->offset = 0;
			} else if (i > 0) {
				memmove(mqp->outbuffer, mqp->outbuffer +i, mqp->outoffset - i);
				mqp->outoffset = mqp->outoffset -i;
			} else {
				/* something went wrong sending the data */
				if (mqpconfig.logger)
					mqpconfig.logger("Error Sending on fd %d", mqp->sock);
				close_fd(mqp->sock, mqp);
				return NS_FAILURE;
			}
		} else {
			/* somethign went wrong encoding the data */
			if (mqpconfig.logger) 
				mqpconfig.logger("Error Encoding the data on fd %d", mqp->sock);
			close_fd(mqp->sock, mqp);
			return NS_FAILURE;
		}
	}
	return NS_SUCCESS;
}		