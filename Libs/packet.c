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


int close_fd (int fd, mqprotocol * mqp);
int buffer_add (mqprotocol * mqp, void *data, size_t datlen);
void buffer_del (mqprotocol * mqp, size_t drainlen);
int sqlite_encode_binary (const unsigned char *in, int n, unsigned char *out);
int listen_on_port (int port);
int pck_accept_connection (int fd);



myengines enc_xdr_engines[NUMENGINES] = {
	{ xdr_encode_uint32, "uint32" },
	{ xdr_encode_int32, "int32" },
	{ xdr_encode_uint64, "unit64" },
	{ xdr_encode_int64, "int64" },
	{ xdr_encode_float, "float" },
	{ xdr_encode_double, "double" },
	{ xdr_encode_octetstream, "octet" },
	{ xdr_encode_string, "string" },
	{ encode_mqs_header, "mqpheader" },
};

myengines dec_xdr_engines[NUMENGINES] = {
	{ xdr_decode_uint32, "uint32" },
	{ xdr_decode_int32, "int32" },
	{ xdr_decode_uint64, "unit64" },
	{ xdr_decode_int64, "int64" },
	{ xdr_decode_float, "float" },
	{ xdr_decode_double, "double" },
	{ xdr_decode_octetstream, "octet" },
	{ xdr_decode_string, "string" },
	{ decode_mqs_header, "mqpheader" },
};

myengines enc_xml_engines[NUMENGINES] = {
	{ xml_encode_uint32, "uint32" },
	{ xml_encode_int32, "int32" },
	{ xml_encode_uint64, "unit64" },
	{ xml_encode_int64, "int64" },
	{ xml_encode_float, "float" },
	{ xml_encode_double, "double" },
	{ xml_encode_octetstream, "octet" },
	{ xml_encode_string, "string" },
};

myengines dec_xml_engines[NUMENGINES] = {
	{ xml_decode_uint32, "uint32" },
	{ xml_decode_int32, "int32" },
	{ xml_decode_uint64, "unit64" },
	{ xml_decode_int64, "int64" },
	{ xml_decode_float, "float" },
	{ xml_decode_double, "double" },
	{ xml_decode_octetstream, "octet" },
	{ xml_decode_string, "string" },
};




void
pck_init ()
{
	int i;
	connections = list_create (-1);
	if (mqpconfig.server) {
		i = listen_on_port (mqpconfig.port);
		if (i > 0) {
			mqpconfig.listenfd = i;
		}
	} else {
		mqpconfig.listenfd = -1;
	}
}

void
pck_set_logger (logfunc * logger)
{
	mqpconfig.logger = logger;
}

void
pck_set_server ()
{
	mqpconfig.server = 1;
	mqpconfig.port = 8888;
}

void
pck_set_port (int port)
{
	mqpconfig.port = port;
}

static int
compare_mid (const void *key1, const void *key2)
{
	mqpacket *mqpck = (void *) key1;
	unsigned long mid = (int) key2;

	if (mqpck->MID == mid) {
		return 0;
	} else {
		return -1;
	}
}

static int
compare_fd (const void *key1, const void *key2)
{
	mqprotocol *mqp = (void *) key1;
	unsigned long fd = (int) key2;

	if (mqp->sock == fd) {
		return 0;
	} else {
		return -1;
	}
}

lnode_t *
pck_find_fd_node (unsigned long fd, list_t * queue)
{
	return (list_find (queue, (void *) fd, compare_fd));
}


lnode_t *
pck_find_mid_node (unsigned long MID, list_t * queue)
{
	return (list_find (queue, (void *) MID, compare_mid));
}

void
pck_create_mqpacket (mqpacket * mqpck, int type, xds_mode_t direction)
{
	int i;
	mqpck = malloc (sizeof (mqpacket));
	if (xds_init (&mqpck->xds, direction) != XDS_OK) {
		if (mqpconfig.logger)
			mqpconfig.logger ("xds init failed: %s", strerror (errno));
		free (mqpck);
		mqpck = NULL;
	}
	i = 0;
	switch (type) {
	case ENG_TYPE_XDR:
		while (i < NUMENGINES) {
			if (xds_register (mqpck->xds, dec_xdr_engines[i].myname, dec_xdr_engines[i].ptr, NULL) != XDS_OK) {
				if (mqpconfig.logger)
					mqpconfig.logger ("xds_register failed for %s", dec_xdr_engines[i].myname);
				xds_destroy (mqpck->xds);
				free (mqpck);
				mqpck = NULL;
			}
			i++;
		}
		break;
	case ENG_TYPE_XML:
		while (i < NUMENGINES - 1) {
			if (xds_register (mqpck->xds, dec_xml_engines[i].myname, dec_xml_engines[i].ptr, NULL) != XDS_OK) {
				if (mqpconfig.logger)
					mqpconfig.logger ("xds_register failed for %s", dec_xml_engines[i].myname);
				xds_destroy (mqpck->xds);
				free (mqpck);
				mqpck = NULL;
			}
			i++;
		}
		break;
	default:
		if (mqpconfig.logger)
			mqpconfig.logger ("invalid pck_create_mqpacket type %d", type);
		xds_destroy (mqpck->xds);
		free (mqpck);
		mqpck = NULL;
		return;
	}
}

void
pck_destroy_mqpacket (mqpacket * mqpck, mqprotocol * mqp)
{
	lnode_t *node;

	if (mqp) {
		node = pck_find_mid_node (mqpck->MID, mqp->inpack);
		if (node) {
#ifdef DEBUG
			if (mqpconfig.logger)
				mqpconfig.logger ("Destroy Packet MessageID %lu", mqpck->MID);
#endif
			list_delete (mqp->inpack, node);
			lnode_destroy (node);
		}
	}
	free (mqpck->data);
	xds_destroy (mqpck->xds);
	free (mqpck);

}


mqprotocol *
pck_new_conn (void *cbarg, int type)
{
	mqprotocol *mqp;
	lnode_t *node;

	if (type > 0 && type < 4) {
		mqp = malloc (sizeof (mqprotocol));
		bzero (mqp, sizeof (mqprotocol));
		mqp->inpack = list_create (-1);
		mqp->outpack = list_create (-1);
		mqp->cbarg = cbarg;
		mqp->sock = 0;
		mqp->pollopts = 0;
		mqp->servorclnt = type;
		/* we are waiting for a new packet */
		mqp->wtforinpack = 1;
		if (mqpconfig.logger)
			mqpconfig.logger ("New Protocol Struct created");
		node = lnode_create (mqp);
		list_append (connections, node);
		return mqp;
	} else {
		if (mqpconfig.logger)
			mqpconfig.logger ("Invalid Connection Type %d", type);
	}
	return NULL;
}

int
pck_free_conn (mqprotocol * mqp)
{
	lnode_t *node, *node2;
	mqprotocol *mqp2;
	mqpacket *mqpck;

	node = list_first (connections);
	while (node != NULL) {
		mqp2 = lnode_get (node);
		if (mqp2 == mqp) {
			/* its a match */
			list_delete (connections, node);
			lnode_destroy (node);
			node2 = list_first (mqp->inpack);
			while (node2 != NULL) {
				mqpck = lnode_get (node2);
				free (mqpck->data);
				xds_destroy (mqpck->xds);
				free (mqpck);
				node2 = list_next (mqp->inpack, node2);
			}
			node2 = list_first (mqp->outpack);
			while (node2 != NULL) {
				mqpck = lnode_get (node2);
				free (mqpck->data);
				xds_destroy (mqpck->xds);
				free (mqpck);
				node2 = list_next (mqp->outpack, node2);
			}
			list_destroy_nodes (mqp->inpack);
			list_destroy_nodes (mqp->outpack);
			list_destroy (mqp->inpack);
			list_destroy (mqp->outpack);
			free (mqp);
			if (mqpconfig.logger)
				mqpconfig.logger ("Deleted Protocol Struct");
			return NS_SUCCESS;
		}
	}
	if (mqpconfig.logger)
		mqpconfig.logger ("Couldn't Find Protocol Struct for deletion");
	return NS_FAILURE;
}

int
read_fd (int fd, void *arg)
{
	void *buf[BUFSIZE];
	mqprotocol *mqp;
	int i;
	lnode_t *node;


	node = pck_find_fd_node (fd, connections);
	if (node) {
		mqp = lnode_get (node);
		bzero (buf, BUFSIZE);
		i = read (fd, buf, BUFSIZE);
		if (i < 1) {
			/* error */
			close_fd (fd, mqp);
			/* XXX close and clean up */
			return NS_FAILURE;
		} else {
			buffer_add (mqp, buf, i);
		}

		i = pck_parse_packet (mqp, mqp->buffer, mqp->offset);
		if (mqpconfig.logger)
			mqpconfig.logger ("processed %d bytes %d left", i, mqp->offset);
		if (i > 0) {
			buffer_del (mqp, i);
		} else if (i == -1) {
			close_fd (fd, mqp);
			return NS_FAILURE;
		}
	} else {
		if (mqpconfig.logger)
			mqpconfig.logger ("Can't find client for %d", fd);
		close_fd (fd, NULL);
		return NS_FAILURE;
	}
	return NS_SUCCESS;
}

int
close_fd (int fd, mqprotocol * mqp)
{
	lnode_t *node;
	if (!mqp) {
		node = pck_find_fd_node (fd, connections);
		if (node)
			mqp = lnode_get (node);
	}
	if (mqp) {
		pck_free_conn (mqp);
	} else {
		if (mqpconfig.logger)
			mqpconfig.logger ("Closing a FD without a connection?");
	}
	close (fd);
	return NS_SUCCESS;
}


int
buffer_add (mqprotocol * mqp, void *data, size_t datlen)
{
	size_t need = mqp->offset + datlen;
//      size_t oldoff = mqp->offset;

	if (mqp->bufferlen < need) {
		void *newbuf;
		int length = mqp->bufferlen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if ((newbuf = realloc (mqp->buffer, length)) == NULL)
			return (-1);
		mqp->buffer = newbuf;
		mqp->bufferlen = length;
	}

	memcpy (mqp->buffer + mqp->offset, data, datlen);
	mqp->offset += datlen;

#if 0
	if (datlen && mqp->cb != NULL)
		(*mqp->cb) (mqp, oldoff, mqp->off, mqp->cbarg);
#endif
	return (0);
}

void
buffer_del (mqprotocol * mqp, size_t drainlen)
{

	memmove (mqp->buffer, mqp->buffer + drainlen, mqp->offset - drainlen);
	mqp->offset = mqp->offset - drainlen;
}

int
write_fd (int fd, mqprotocol * mqp)
{
	int i;
	lnode_t *node;
	mqpacket *mqpacket;
	if (mqp) {
		node = pck_find_fd_node (fd, connections);
		if (node) {
			mqp = lnode_get (node);
		} else {
			if (mqpconfig.logger)
				mqpconfig.logger ("Can not find mqprotocol for fd %d", fd);
			return NS_FAILURE;
		}
	}

	if (mqp->outbufferlen > 0) {
		i = write (mqp->sock, mqp->outbuffer, mqp->outbufferlen);
		if (i == mqp->outbufferlen) {
			free (mqp->outbuffer);
			mqp->outbufferlen = mqp->outoffset = 0;
			mqp->pollopts |= ~POLLOUT;
		} else if (i > 0) {
			memmove (mqp->outbuffer, mqp->outbuffer + i, mqp->outoffset - i);
			mqp->outoffset = mqp->outoffset - i;
			mqp->pollopts = POLLOUT;
		} else {
			/* something went wrong sending the data */
			if (mqpconfig.logger)
				mqpconfig.logger ("Error Sending on fd %d", mqp->sock);
			close_fd (mqp->sock, mqp);
			return NS_FAILURE;
		}
	}
	if (list_count (mqp->outpack) > 0) {
		node = list_first (mqp->outpack);
		mqpacket = lnode_get (node);
		mqp->outbuffer = malloc (2 + (257 * mqpacket->dataoffset) / 254);
		mqp->outbufferlen = sqlite_encode_binary (mqpacket->data, mqpacket->dataoffset, mqp->outbuffer);
		mqp->outoffset = mqp->outbufferlen;
		if (mqp->outbufferlen > 0) {
			list_delete (mqp->outpack, node);
			lnode_destroy (node);
			i = write (mqp->sock, mqp->outbuffer, mqp->outbufferlen);
			if (i == mqp->outbufferlen) {
				free (mqp->outbuffer);
				mqp->bufferlen = mqp->offset = 0;
				mqp->pollopts |= ~POLLOUT;
			} else if (i > 0) {
				memmove (mqp->outbuffer, mqp->outbuffer + i, mqp->outoffset - i);
				mqp->outoffset = mqp->outoffset - i;
				mqp->pollopts &= POLLOUT;
			} else {
				/* something went wrong sending the data */
				if (mqpconfig.logger)
					mqpconfig.logger ("Error Sending on fd %d", mqp->sock);
				close_fd (mqp->sock, mqp);
				return NS_FAILURE;
			}
		} else {
			/* somethign went wrong encoding the data */
			if (mqpconfig.logger)
				mqpconfig.logger ("Error Encoding the data on fd %d", mqp->sock);
			close_fd (mqp->sock, mqp);
			return NS_FAILURE;
		}
	}
	return NS_SUCCESS;
}


int
pck_before_poll (struct pollfd *ufds)
{
	lnode_t *node;
	mqprotocol *mqp;
	int count = 0;

	node = list_first (connections);
	while (node != NULL) {
		mqp = lnode_get (node);
		if (mqp->pollopts > 0) {
			ufds = realloc (ufds, count + 1 * sizeof (struct pollfd));
			ufds[count].fd = mqp->sock;
			ufds[count].events = mqp->pollopts;
			count++;
			node = list_next (connections, node);
		}
	}
	/* now do the listen ports */
	if (mqpconfig.listenfd > 0) {
		ufds = realloc (ufds, count + 1 * sizeof (struct pollfd));
		ufds[count].fd = mqpconfig.listenfd;
		/* XXX */
		ufds[count].events = POLLIN | POLLOUT;
		count++;
	}
	return count;
}

int
pck_after_poll (const struct pollfd *ufds, int nfds)
{
	int i;
	for (i = 0; i < nfds; i++) {
		if (mqpconfig.listenfd == ufds[i].fd) {
			pck_accept_connection (ufds[i].fd);
			continue;
		}
		if (ufds[i].revents == POLLHUP || ufds[i].revents == POLLERR) {
			close_fd (ufds[i].fd, NULL);
		}
		if (ufds[i].revents == POLLIN) {
			read_fd (ufds[i].fd, NULL);
		}
		if (ufds[i].revents == POLLOUT) {
			write_fd (ufds[i].fd, NULL);
		}
	}
	return NS_SUCCESS;
}

int
pck_process ()
{
	struct pollfd *ufds = NULL;
	int i, j;

	i = pck_before_poll (ufds);
	j = poll (ufds, i, 100);
	return pck_after_poll (ufds, j);
}

int
listen_on_port (int port)
{
	int srvfd;		/* FD for our listen server socket */
	struct sockaddr_in srvskt;
	int adrlen;
	int flags;

	adrlen = sizeof (struct sockaddr_in);
	(void) memset ((void *) &srvskt, 0, (size_t) adrlen);
	srvskt.sin_family = AF_INET;

#if 0
	/* bind to the local IP */
	if (dobind) {
		srvskt.sin_addr = lsa.sin_addr;
	} else {
#endif
		srvskt.sin_addr.s_addr = INADDR_ANY;
#if 0
	}
#endif

	srvskt.sin_port = htons (me.port);
	if ((srvfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("SqlSrv: Unable to get socket for port %d.", port);
		return NS_FAILURE;
	}
	flags = fcntl (srvfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	(void) fcntl (srvfd, F_SETFL, flags);
//  setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, (char*) 1, sizeof(1));

	if (bind (srvfd, (struct sockaddr *) &srvskt, adrlen) < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("Unable to bind to port %d", port);
		return NS_FAILURE;
	}
	if (listen (srvfd, 1) < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("Unable to listen on port %d", port);
		return -1;
	}
	return (srvfd);
}


int
pck_accept_connection (int fd)
{
	unsigned int al;
	struct sockaddr_in client_address;
	int l;
	mqprotocol *mqp;

	memset (&client_address, 0, al = sizeof (client_address));
	l = accept (fd, (struct sockaddr *) &client_address, &al);
	if (l < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("Accept Failed on %d: %s", fd, strerror (errno));
		return NS_FAILURE;
	}
	mqp = pck_new_conn (NULL, PCK_IS_CLIENT);
	mqp->sock = l;
	mqp->flags = 0;
	if (mqpconfig.connectauth) {
		if (!mqpconfig.connectauth (mqp->sock, client_address)) {
			/* close the sock */
			if (mqpconfig.logger)
				mqpconfig.logger ("Un-Authorized Connection. Dropping %d", l);
			close_fd (l, mqp);
		}
	} else {
		mqp->flags = MQP_CLIENTAUTHED;
	}
	mqp->pollopts |= POLLIN;
	return l;
}

int
pck_make_connection (struct sockaddr_in sa, void *cbarg)
{
	int s;
	int flags;
	mqprotocol *mqp;

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("Can't create socket");
		return NS_FAILURE;
	}
	/* XXX bind */
	flags = fcntl (s, F_GETFL, 0);
	flags |= O_NONBLOCK;
	(void) fcntl (s, F_SETFL, flags);

	flags = connect (s, (struct sockaddr *) &sa, sizeof (sa));
	if (flags < 0) {
		if (mqpconfig.logger)
			mqpconfig.logger ("Connect Failed");
		return NS_FAILURE;
	}
	mqp = pck_new_conn (cbarg, PCK_IS_SERVER);
	mqp->sock = s;
	mqp->flags = 0;
	mqp->pollopts |= POLLOUT;
	return s;
}
