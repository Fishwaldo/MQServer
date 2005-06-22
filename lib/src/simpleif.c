/* MQServer - Abstract message Processing Server
** Copyright (c) 2005 Justin Hammond
** http://www.mqserver.info/
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
** MQServer CVS Identification
** $Id$
*/


#include "libmq.h"
#include "list.h"
#include "packet.h"
#include "simpleif.h"

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

list_t *connections;

typedef struct sc {
	int listenfd;
	int listport;
	mqp *mqplib;
	int debug;
	void *cbarg;
	actioncbfunc *actioncb;
} sc;
sc sockconfig;

int pck_accept_connection (int fd);
int listen_on_port (int port);
void pck_logger(char *fmt, ...);


int pck_simple_callback(void *mqplib, mqpacket *mqp) {
	int type = 0;

	switch (mqp->inmsg.MSGTYPE) {
		case PCK_ACK:
			if (MQS_S_FLAG_IS_GOTSRVCAP(mqp)) {
				TRACE(mqplib, MQDBG2, "Got CLNTCAP ACK");
				MQS_S_FLAG_SET_SENTAUTH(mqp);
				pck_send_auth(mqplib, mqp, mqp->si.username, mqp->si.password);
			} else if (MQS_S_FLAG_IS_SENTAUTH(mqp)) {
				TRACE(mqplib, MQDBG2, "login Ack'd");
				type = PCK_SMP_LOGINOK;
				MQS_S_FLAG_SET_CONNECTOK(mqp);
			}
			break;
		case PCK_SRVCAP:
			TRACE(mqplib, MQDBG2, "Got ServerCap");
			MQS_S_FLAG_SET_GOTSRVCAP(mqp);
			pck_send_clntcap(mqplib, mqp);
			break;
		case PCK_ERROR:
			if (MQS_S_FLAG_IS_GOTSRVCAP(mqp)) {
				MQLOG(mqplib, MQLOG_CRITICAL, "Server rejected out clientcap for %s", mqp->inmsg.data.string);
/* 				type = PCK_SMP_CLNTCAPREJ; */
				return NS_FAILURE;
			} else if (MQS_S_FLAG_IS_SENTAUTH(mqp)) {
				MQLOG(mqplib, MQLOG_CRITICAL, "Server rejected out Login: %s", mqp->inmsg.data.string);
/* 				type = PCK_SMP_AUTHREJ; */
				return NS_FAILURE;
			}
			break;
		case PCK_QUEUEINFO:
			TRACE(mqplib, MQDBG2, "Got QueueInfo for %s: %ld (%s)", mqp->inmsg.data.joinqueue.queue, mqp->inmsg.data.joinqueue.flags, mqp->inmsg.data.joinqueue.filter);
			type = PCK_SMP_QUEUEINFO;
			break;
		case PCK_MSGFROMQUEUE:
			TRACE(mqplib, MQDBG2, "Got Message from Queue %s sent by %s on %ld with topic %s with messid %ld", mqp->inmsg.data.sendmsg.queue, mqp->inmsg.data.sendmsg.from, mqp->inmsg.data.sendmsg.timestamp, mqp->inmsg.data.sendmsg.topic, mqp->inmsg.data.sendmsg.messid);
			type = PCK_SMP_MSGFROMQUEUE;
			break;
		default:
			MQLOG(mqplib, MQLOG_WARNING, "Uknown msgtype recieved: %xd", mqp->inmsg.MSGTYPE);
	}			

	if (type != 0) {
		(sockconfig.actioncb) (type, sockconfig.cbarg);
	}

	return NS_SUCCESS;
}


int init_socket(actioncbfunc *actioncb) {
	connections = list_create(-1);
	sockconfig.listenfd = -1;
	sockconfig.listport = -1;
	sockconfig.mqplib = init_mqlib();
	sockconfig.actioncb = actioncb;
	pck_set_callback(sockconfig.mqplib, pck_simple_callback);
	return NS_SUCCESS;
}

int debug_socket(int i) {
	pck_set_dbglvl(sockconfig.mqplib, i);
	return i;
}

int enable_server(int port) {
	sockconfig.listport = 8889;
	return NS_SUCCESS;
}

void 
pck_fini() {
	lnode_t *node;
	mqpacket *mqp;

	node = list_first (connections);
	while (node != NULL) {
		mqp = lnode_get (node);
		close_fd(sockconfig.mqplib, mqp);
		node = list_next(connections, node);
	}
	list_destroy_nodes(connections);
	list_destroy(connections);
	fini_mqlib(sockconfig.mqplib);
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
			ufds[count].revents = 0;
			count++;
		}
		node = list_next (connections, node);
		if (count == (MAXCONNECTIONS -1)) {
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
	
	if (nfds == 0) {
		return NS_SUCCESS;
	}
	for (i = 0; i <= (nfds-1); i++) {
		if (sockconfig.listenfd == ufds[i].fd) {
			pck_accept_connection (ufds[i].fd);
			continue;
		}
#if 0
		/* this isn't needed I guess, read and write can detect EOF */
		if (ufds[i].revents & POLLHUP || ufds[i].revents & POLLERR) {
		if (ufds[i].revents & POLLHUP) {
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				close_fd (sockconfig.mqplib, mqp);
				list_delete(connections, node);
				lnode_destroy(node);
			}
			return NS_FAILURE;
		}
#endif
		if (ufds[i].revents & POLLIN) {
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				if (read_fd (sockconfig.mqplib, mqp) != NS_SUCCESS) {
					list_delete(connections, node);
					lnode_destroy(node);
					return NS_FAILURE;
				}			
			}
		}
		if (ufds[i].revents & POLLOUT) {
			node = pck_find_fd_node(ufds[i].fd, connections);
			if (node) {
				mqp = lnode_get(node);
				if (write_fd (sockconfig.mqplib, mqp) != NS_SUCCESS) {
					list_delete(connections, node);
					lnode_destroy(node);
					return NS_FAILURE;
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
pck_make_connection (char *hostname, char *username, char *password, long flags, void *cbarg, actioncbfunc *actioncb)
{
	int s;
	int myflags;
	mqpacket *mqp;
	lnode_t *node;
	struct sockaddr_in sa;
	struct hostent *hp;
	struct in_addr ipad;

	init_socket(actioncb);
	debug_socket(1);

	if (inet_aton(hostname, &ipad) == 0) {
	
		if ((hp = gethostbyname(hostname)) == NULL) {
			/* XXX Err vals */
			return NS_FAILURE;
		}
		sa.sin_family = AF_INET;
		sa.sin_port = htons(8889);
		bcopy(hp->h_addr, (char *) &sa.sin_addr, hp->h_length);
	} else {
		sa.sin_addr.s_addr = ipad.s_addr;
	}
	
	

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		/* XXX Err Vals */
		return NS_FAILURE;
	}
	/* XXX bind */
	myflags = fcntl (s, F_GETFL, 0);
	myflags |= O_NONBLOCK;
	(void) fcntl (s, F_SETFL, myflags);

	flags = connect (s, (struct sockaddr *) &sa, sizeof (sa));
	if (flags < 0) {
		if (errno != EINPROGRESS) {
			/* XXX Err Vals */
			return NS_FAILURE;
		}
	}
	mqp = pck_new_connection (sockconfig.mqplib, s, ENG_TYPE_XML, PCK_IS_SERVER);
	mqp->si.username = (mqlib_malloc)(BUFSIZE);
	mqp->si.password = (mqlib_malloc)(BUFSIZE);	

	snprintf(mqp->si.username, BUFSIZE, "%s", username);
	snprintf(mqp->si.password, BUFSIZE, "%s", password);
	snprintf(mqp->si.host, BUFSIZE, "%s", hostname);
	mqp->si.flags = 0;


	node = lnode_create(mqp);
	list_append(connections, node);

	mqp->pollopts |= POLLIN;
	
	return s;
}

unsigned long
pck_simple_send_message_struct(int conid, structentry *mystruct, int cols, void *data, char *destination, char *topic) {
	lnode_t *node;
	mqpacket *mqp;

	node = pck_find_fd_node(conid, connections);
	if (node) {
		mqp = lnode_get(node);
		return(pck_send_message_struct(sockconfig.mqplib, mqp, mystruct, cols, data, destination, topic));
	}
	return NS_FAILURE;
}
unsigned long
pck_simple_joinqueue(int conid, char *queue, int flags, char *filter) {
	lnode_t *node;
	mqpacket *mqp;

	node = pck_find_fd_node(conid, connections);
	if (node) {
		mqp = lnode_get(node);
		return (pck_send_joinqueue(sockconfig.mqplib, mqp, queue, flags, filter));
	}
	return NS_FAILURE;
}

/* this can only be called when we get a QUEUEINFO message */
mq_data_joinqueue
*pck_get_queueinfo(int conid) {
	lnode_t *node;
	mqpacket *mqp;

	node = pck_find_fd_node(conid, connections);
	if (node) {
		mqp = lnode_get(node);
		if (mqp->inmsg.MSGTYPE != PCK_QUEUEINFO) {
			return NULL;
		}
		return &mqp->inmsg.data.joinqueue;
	}
	return NULL;
}

/* this can only be called when we get a MSGFROMQUEUE message */
mq_data_senddata 
*pck_get_msgfromqueue(int conid) {
	lnode_t *node;
	mqpacket *mqp;

	node = pck_find_fd_node(conid, connections);
	if (node) {
		mqp = lnode_get(node);
		if (mqp->inmsg.MSGTYPE != PCK_MSGFROMQUEUE) {
			return NULL;
		}
		return &mqp->inmsg.data.sendmsg;
	}
	return NULL;
}

int 
pck_decode_message(mq_data_senddata *sd, structentry *mystruct, int cols, void *target) {
	xds_t *xdstmp;
	void *destptr;
	char *string;
	int i, rc, myint;

	
	xdstmp = pck_init_engines(sockconfig.mqplib, ENG_TYPE_XML, XDS_DECODE);
	if (xdstmp == NULL)  {
/* 		pck_logger("pck_decode_message xds init failed"); */
		return NS_FAILURE;
	}

	if (xds_setbuffer (xdstmp, XDS_LOAN, sd->data, sd->len) != XDS_OK) {
/* 		pck_logger("pck_decode_message XDS setbuffer Failed"); */
		xds_destroy(xdstmp);
	        return NS_FAILURE;
	}

	for (i = 0; i < cols; i++) {
		switch (mystruct[i].type) {
			case STR_PSTR:
				rc = xds_decode(xdstmp, "string", &string);
				if (rc == XDS_OK) {
					destptr = (void *) target + mystruct[i].offset;
					*(char **)destptr = strndup(string, strlen(string));
				} else {
/* 					pck_logger("pck_decode_message, STR_PSTR failed\n"); */
				}			
				free(string);
				break;
			case STR_INT:
				rc = xds_decode(xdstmp, "int32", &myint);
				if (rc == XDS_OK) {
					destptr = (void *) target + mystruct[i].offset;
					*((int *)destptr) = myint;
				} else {
/*					pck_logger("pck_decode_message, STR_INT failed\n"); */
				}			
				break;
			case STR_STR:
				rc = xds_decode(xdstmp, "string", &string);
				if (rc == XDS_OK) {
					destptr = (void *) target + mystruct[i].offset;
					strncpy((char *)destptr, string, mystruct[i].size);			
				} else {
/* 					pck_logger("pck_decode_message, STR_STR failed\n"); */
				}			
				free(string);
				break;
			default:
/* 				pck_logger( "pck_decode_message, unknown type"); */
				break;
		}
	}
	xds_destroy(xdstmp);
	return NS_SUCCESS;
}
