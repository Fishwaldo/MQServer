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


list_t *connections;

typedef struct sc {
	int listenfd;
	int listport;
	mqp *mqplib;
	int debug;
} sc;
sc sockconfig;

int pck_accept_connection (int fd);
int listen_on_port (int port);


int pck_simple_callback(void *mqplib, mqpacket *mqp) {








}

void pck_logger(char *fmt,...) {
	va_list ap;
	char log_buf[BUFSIZE];
	if (sockconfig.debug >= 1) {
		va_start(ap, fmt);
		vsnprintf(log_buf, BUFSIZE, fmt, ap);
		va_end(ap);
		printf("MQ: %s\n", log_buf);
	}
}

int init_socket() {
	connections = list_create(-1);
	sockconfig.listenfd = -1;
	sockconfig.listport = -1;
	sockconfig.mqplib = init_mqlib();
	sockconfig.debug = 0;
	pck_set_logger(sockconfig.mqplib, pck_logger);		
	pck_set_callback(sockconfig.mqplib, pck_simple_callback);
	return NS_SUCCESS;
}

int debug_socket(int i) {
	sockconfig.debug = i;
	return i;
}

int enable_server(int port) {
	sockconfig.listport = 8888;
	return NS_SUCCESS;
}


int
pck_before_poll (struct pollfd ufds[MAXCONNECTIONS])
{
	lnode_t *node;
	mqpacket *mqp;
	int count = 0;

	node = list_first (connections);
	while (node != NULL) {
		mqp = lnode_get (node);
		if (mqp->pollopts > 0) {
			//ufds = realloc (ufds, count + 1 * sizeof (struct pollfd));
			ufds[count].fd = mqp->sock;
			ufds[count].events = mqp->pollopts;
			count++;
		}
		node = list_next (connections, node);
		if (count == (MAXCONNECTIONS -1)) {
			printf("count %d\n", count);
			break;
		}
	}
	/* now do the listen ports */
	if (sockconfig.listenfd > 0) {
		//ufds = realloc (ufds, count + 1 * sizeof (struct pollfd));
		ufds[count].fd = sockconfig.listenfd;
		/* XXX */
		ufds[count].events = POLLIN;
		count++;
	}
	return count;
}


static int
compare_fd (const void *key1, const void *key2)
{
	mqpacket *mqp = (void *) key1;
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


int
pck_after_poll (const struct pollfd *ufds, int nfds)
{
	int i;
	lnode_t *node;
	mqpacket *mqp;
	
	printf("ready: %d\n", nfds);
	if (nfds == 0) {
		return NS_SUCCESS;
	}
	for (i = 0; i <= nfds; i++) {
		printf("checking fd %d %d\n", ufds[i].fd, ufds[i].revents);
		if (sockconfig.listenfd == ufds[i].fd) {
			pck_accept_connection (ufds[i].fd);
			continue;
		}
		if (ufds[i].revents & POLLHUP || ufds[i].revents & POLLERR) {
			printf("close %d\n", ufds[i].fd);
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				close_fd (sockconfig.mqplib, mqp);
				list_delete(connections, node);
				lnode_destroy(node);
			}
		}
		if (ufds[i].revents & POLLIN) {
			printf("readfd %d\n", ufds[i].fd);
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				if (read_fd (sockconfig.mqplib, mqp) != NS_SUCCESS) {
					printf("dropping connection after read\n");
					list_delete(connections, node);
					lnode_destroy(node);
				}			
			}
		}
		if (ufds[i].revents & POLLOUT) {
			printf("writefd %d\n", ufds[i].fd);
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				if (write_fd (sockconfig.mqplib, mqp) != NS_SUCCESS) {
					list_delete(connections, node);
					lnode_destroy(node);
				}
			}
		}
	}
	return NS_SUCCESS;
}

int
pck_process ()
{
	struct pollfd ufds[MAXCONNECTIONS];
	int i, j;

	/* first thing we do is see if we are meant to be a server */
	if (sockconfig.listport != -1 && sockconfig.listenfd == -1) {
		i = listen_on_port (sockconfig.listport);
		if (i > 0) {
			sockconfig.listenfd = i;
		} else {
			sockconfig.listenfd = -1;
		}
	}		

	i = pck_before_poll (ufds);
	j = poll (ufds, i, 100);

	if (j < 0) {
		printf("poll returned %d: %s\n", j, strerror(j));
		return NS_FAILURE;
	}
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
	srvskt.sin_addr.s_addr = INADDR_ANY;
	srvskt.sin_port = htons (port);


	if ((srvfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf ("SqlSrv: Unable to get socket for port %d.", port);
		return NS_FAILURE;
	}
	flags = fcntl (srvfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
// 	(void) fcntl (srvfd, F_SETFL, flags);
//  setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, (char*) 1, sizeof(1));

	if (bind (srvfd, (struct sockaddr *) &srvskt, adrlen) < 0) {
		printf ("Unable to bind to port %d\n", port);
		return NS_FAILURE;
	}
	if (listen (srvfd, 1) < 0) {
		printf ("Unable to listen on port %d\n", port);
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
	mqpacket *mqp;
	lnode_t *node;
	
	memset (&client_address, 0, al = sizeof (client_address));
	l = accept (fd, (struct sockaddr *) &client_address, &al);
	if (l < 0) {
		printf ("Accept Failed on %d: %s\n", fd, strerror (errno));
		return NS_FAILURE;
	}
	mqp = pck_new_connection (sockconfig.mqplib, l, ENG_TYPE_XDR, PCK_IS_CLIENT);
#if 0
	if (sockconfig.connectauth) {
		if (!sockconfig.connectauth (mqp->sock, client_address)) {
			/* close the sock */
			printf ("Un-Authorized Connection. Dropping %d\n", l);
			close_fd (l, mqp);
		}
	} else {
		MQP_SET_AUTHED(mqp);
	}
#endif
	printf ("Connection on fd %d\n", l);
		
	mqp->pollopts |= POLLIN;

	node = lnode_create(mqp);
	list_append(connections, node);




	return l;
}

int
pck_make_connection (char *hostname, char *username, char *password, long flags, void *cbarg)
{
	int s;
	int myflags;
	mqpacket *mqp;
	lnode_t *node;
	struct sockaddr_in sa;
	struct hostent *hp;
	struct in_addr ipad;

	init_socket();
	debug_socket(1);

	if (inet_aton(hostname, &ipad) == 0) {
	
		if ((hp = gethostbyname(hostname)) == NULL) {
			printf("gethostbyname failed\n");
			return NS_FAILURE;
		}
		sa.sin_family = AF_INET;
		sa.sin_port = htons(8888);
		bcopy(hp->h_addr, (char *) &sa.sin_addr, hp->h_length);
	} else {
		sa.sin_addr.s_addr = ipad.s_addr;
	}
	
	

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		printf ("Can't create socket\n");
		return NS_FAILURE;
	}
	/* XXX bind */
	myflags = fcntl (s, F_GETFL, 0);
	myflags |= O_NONBLOCK;
	(void) fcntl (s, F_SETFL, myflags);

	flags = connect (s, (struct sockaddr *) &sa, sizeof (sa));
	if (flags < 0) {
		if (errno != EINPROGRESS) {
			printf ("Connect Failed %s\n", strerror(errno));
			return NS_FAILURE;
		}
	}
	mqp = pck_new_connection (sockconfig.mqplib, s, ENG_TYPE_XDR, PCK_IS_SERVER);

	snprintf(mqp->si.username, BUFSIZE, "%s", username);
	snprintf(mqp->si.password, BUFSIZE, "%s", password);
	snprintf(mqp->si.host, BUFSIZE, "%s", hostname);
	mqp->si.flags = 0;


	node = lnode_create(mqp);
	list_append(connections, node);


	mqp->pollopts |= POLLIN;
	printf("OutGoing Connection on fd %d\n", s);
	return s;
}


