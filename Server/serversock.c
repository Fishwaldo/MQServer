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
** $Id: sock.c 21 2004-07-27 06:03:37Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "dotconf.h"
#include "packet.h"
#include "dns.h"
#include "serversock.h"
#include "xds.h"
#include "admincli.h"
#include "callback.h"
#include "mythread.h"

int MQS_listen_on_port(int port, long type);



void MQS_Logger(char *fmt,...) {
        va_list ap;
        char log_buf[BUFSIZE];
        va_start(ap, fmt);
        vsnprintf(log_buf, BUFSIZE, fmt, ap);
        va_end(ap);
	nlog(LOG_DEBUG1, LOG_CORE, "%s", log_buf);
}

void MQS_remove_client(mqsock *mqs) {
	nlog(LOG_DEBUG1, LOG_CORE, "Dropping client %d", mqs->data.fd);
	event_del(&mqs->ev);
	list_delete(mqssetup.connections, mqs->node);
	lnode_destroy(mqs->node);
	free(mqs);
}	


void MQS_client_activity(int fd, short eventtype, void *arg) {
	mqsock *mqs = arg;
	int rc = -1;
	
	SET_SEGV_LOCATION();
	if (eventtype == EV_READ) {
		MQS_Logger("Reading %d for Data", fd);
		rc = read_fd(mqssetup.mqplib, mqs->mqp);
	} else if (eventtype == EV_WRITE) {
		MQS_Logger("Writting %d for Data", fd);
		rc = write_fd(mqssetup.mqplib, mqs->mqp);
	}
	if (rc == NS_FAILURE) {
		MQS_remove_client(mqs);			
	}

}

void spawn_admin_thread(mqsock *mqs) {
	create_thread("AdminCLI", init_admin_cli, (void *)mqs->data.fd);
}

static int compare_conid(const void *arg1, const void *arg2) {
	mqsock *mqs = (mqsock *)arg1;
	int conid = (int)arg2;
printf("%d - %d\n", mqs->connectid, conid);
	if (mqs->connectid == conid) {
		return 0;
	} else {
		return -1;
	}
}

extern mqsock *find_con_by_id(int id) {
	lnode_t *node;
	mqsock *mqs;
	node = list_find(mqssetup.connections, (void *)id, compare_conid);
	if (node) {
		mqs = lnode_get(node);
		if (!mqs->mqp) {
			printf("hrm, mqp missing\n");
		}
printf("return ok\n");
		return lnode_get(node);
	} 
	return NULL;
}

void MQS_listen_accept(int fd, short eventtype, void *arg) {
        unsigned int al;
        struct sockaddr_in client_address;
        int l;
	mqsock *mqs = arg;
	mqsock *newmqs;
	
	SET_SEGV_LOCATION();
	newmqs = malloc(sizeof(mqsock));
	switch (mqs->type) {
	  	case MQC_TYPE_LISTEN_XDR:
		  	MQC_SET_TYPE_CLIENT_XDR(newmqs);
	  		break;
  		case MQC_TYPE_LISTEN_XML:
 	  		MQC_SET_TYPE_CLIENT_XML(newmqs);
 			break;
	 	case MQC_TYPE_LISTEN_ADMIN:
 			MQC_SET_TYPE_ADMIN(newmqs);
	 		break;
 		default:
 			nlog(LOG_CRITICAL, LOG_CORE, "Invalid Listen type %ld", mqs->type);
			free(newmqs);
	 		return;
 			break;
	}	

        memset (&client_address, 0, al = sizeof (client_address));
        l = accept (fd, (struct sockaddr *)&client_address, &al);
	if (l < 0) {
		nlog(LOG_WARNING, LOG_CORE, "Accept Failed on %d: %s", fd, strerror(errno));
		return;
	}
 	MQC_SET_STAT_CONNECT(newmqs);
	newmqs->ip = client_address;
	strncpy(newmqs->host, inet_ntoa(client_address.sin_addr), MAXHOST);
	newmqs->data.fd = l;
	newmqs->connectid = mqssetup.nxtconid++;
	nlog(LOG_DEBUG1, LOG_CORE, "New Connection from %s on fd %d", newmqs->host, newmqs->data.fd);
	/* start DNS lookup */
	do_reverse_lookup(newmqs);

}


void got_dns(mqsock *mqs) {
	if (MQC_IS_TYPE_CLIENT_XDR(mqs)) {
		mqs->mqp = pck_new_connection(mqssetup.mqplib, mqs->data.fd, ENG_TYPE_XDR, PCK_IS_CLIENT);
		pck_set_data(mqs->mqp, (void *)mqs);
		event_set(&mqs->ev, mqs->data.fd, EV_READ|EV_PERSIST, MQS_client_activity, mqs);
		event_add(&mqs->ev, NULL);	
		mqs->node = lnode_create(mqs);
  		list_append(mqssetup.connections, mqs->node);
	} else if (MQC_IS_TYPE_CLIENT_XML(mqs)) {
		mqs->mqp = pck_new_connection(mqssetup.mqplib, mqs->data.fd, ENG_TYPE_XML, PCK_IS_CLIENT);
		pck_set_data(mqs->mqp, (void *)mqs);
		event_set(&mqs->ev, mqs->data.fd, EV_READ|EV_PERSIST, MQS_client_activity, mqs);
		event_add(&mqs->ev, NULL);	
		mqs->node = lnode_create(mqs);
  		list_append(mqssetup.connections, mqs->node);
	} else if (MQC_IS_TYPE_ADMIN(mqs)) {
		/* XXX Admin thread needs to signal when finished */
		spawn_admin_thread(mqs);
		mqs->node = lnode_create(mqs);
  		list_append(mqssetup.connections, mqs->node);
	} else {
		printf("this is fubar\n");
	}
}


/** @brief main recv loop
 *
 * @param none
 * 
 * @return none
 */
void
MQS_sock_start ()
{
	
	struct event dnstimeout;
	struct timeval tv;
	
	SET_SEGV_LOCATION();

	mqssetup.connections = list_create(-1);

	mqssetup.mqplib = init_mqlib();
	pck_set_logger(mqssetup.mqplib, MQS_Logger);
	pck_set_callback(mqssetup.mqplib, MQS_Callback);

	event_init();

	mqssetup.xdrport = 8888;
	mqssetup.xmlport = 8889;
	mqssetup.adminport = 8887;
	mqssetup.nxtconid = 0;

	if (MQS_listen_on_port(mqssetup.xdrport, MQC_TYPE_LISTEN_XDR) != NS_SUCCESS) {
		do_exit(NS_EXIT_ERROR, "Can't Create Client Port");
		return;
	}

	if (MQS_listen_on_port(mqssetup.xmlport, MQC_TYPE_LISTEN_XML) != NS_SUCCESS) {
		do_exit(NS_EXIT_ERROR, "Can't Create Client Port");
		return;
	}
	
	if (MQS_listen_on_port(mqssetup.adminport, MQC_TYPE_LISTEN_ADMIN) != NS_SUCCESS) {
		do_exit(NS_EXIT_ERROR, "Can't create Admin Port");
		return;
	}
	
	/* setup DNS to run every so often */
	evtimer_set(&dnstimeout, check_dns, &dnstimeout);
	timerclear(&tv);
	tv.tv_sec = 1;	
	event_add(&dnstimeout, &tv);
	
	while (1) {
		SET_SEGV_LOCATION();

/* 		setup_dns_socks(); */

		event_loop(EVLOOP_ONCE);
		
		/* check our queues */
		check_authq();	
	
		if (me.die) {
			do_exit(NS_EXIT_NORMAL, "Normal Exit");
		}
	}

	do_exit(NS_EXIT_ERROR, "Exit from Loop");
}



/***************************************************************
 * listen_on_port(int port): -  Open a socket to listen for
 * incoming TCP connections on the port given.  Return the file
 * descriptor if OK, and -1 on any error.  The calling routine
 * can handle any error condition.
 *
 * Input:        The interger value of the port number to bind to
 * Output:       The file descriptor of the socket
 * Effects:      none
 ***************************************************************/
int
MQS_listen_on_port(int port, long type)
{
  int      srvfd;      /* FD for our listen server socket */
  struct sockaddr_in srvskt;
  int      adrlen;
  int      flags;
  mqsock   *mqs;
  int 	   on = 1;

  SET_SEGV_LOCATION();

  mqs = malloc(sizeof(mqsock));
  switch (type) {
  	case MQC_TYPE_LISTEN_XDR:
	  	MQC_SET_TYPE_LISTEN_XDR(mqs);
  		break;
  	case MQC_TYPE_LISTEN_XML:
 	  	MQC_SET_TYPE_LISTEN_XML(mqs);
 		break;
 	case MQC_TYPE_LISTEN_ADMIN:
 		MQC_SET_TYPE_LISTEN_ADMIN(mqs);
 		break;
 	default:
 		nlog(LOG_CRITICAL, LOG_CORE, "Invalid Listen type %ld", type);
 		return NS_FAILURE;
 		break;
 }


  adrlen = sizeof(struct sockaddr_in);
  (void) memset((void *) &srvskt, 0, (size_t) adrlen);
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
  srvskt.sin_port = htons(port);
  if ((srvfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    nlog(LOG_CRITICAL,LOG_CORE, "SqlSrv: Unable to get socket for port %d.", port);
    return NS_FAILURE;
  }
  flags = fcntl(srvfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  (void) fcntl(srvfd, F_SETFL, flags);
  setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

  if (bind(srvfd, (struct sockaddr *) &srvskt, adrlen) < 0)
  {
    nlog(LOG_CRITICAL, LOG_CORE, "Unable to bind to port %d", port);
    return NS_FAILURE;
  }
  if (listen(srvfd, 1) < 0)
  {
    nlog(LOG_CRITICAL, LOG_CORE, "Unable to listen on port %d", port);
    return -1;
  }
  mqs->data.fd = srvfd;
  event_set(&mqs->ev, mqs->data.fd, EV_READ|EV_PERSIST, MQS_listen_accept, mqs);
  event_add(&mqs->ev, NULL);	
  nlog(LOG_DEBUG1, LOG_CORE, "Listening on %d (%d) for %x connections", port, srvfd, type);

  mqs->node = lnode_create(mqs);
  list_append(mqssetup.connections, mqs->node);
  return NS_SUCCESS;
}




