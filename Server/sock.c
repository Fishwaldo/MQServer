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
#include "dotconf.h"
#include "client.h"
#include "dns.h"

static struct sockaddr_in lsa;
static int dobind;

int servsock;

static int listen_on_port(int port);

static void setup_dns_socks();

static int buffer_add(mqclient *mqc, void *data, size_t datlen);


/** @brief Connect to a server
 *
 *  also setups the SQL listen socket if defined 
 *
 * @param host to connect to
 * @param port on remote host to connect to
 * 
 * @return socket connected to on success
 *         NS_FAILURE on failure 
 */
int
ConnectTo (char *host, int port)
{
	int ret;
	struct hostent *hp;
	struct sockaddr_in sa;
	int s;

	SET_SEGV_LOCATION();
	dobind = 0;
	/* bind to a local ip */
	memset (&lsa, 0, sizeof (lsa));
	if (me.local[0] != 0) {
		if ((hp = gethostbyname (me.local)) == NULL) {
			nlog (LOG_WARNING, LOG_CORE, "Warning, Couldn't bind to IP address %s", me.local);
		} else {
			memcpy ((char *) &lsa.sin_addr, hp->h_addr, hp->h_length);
			lsa.sin_family = hp->h_addrtype;
			dobind = 1;
		}
	}
	if ((hp = gethostbyname (host)) == NULL) {
		return NS_FAILURE;
	}

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		free(hp);
		return NS_FAILURE;
	}
	if (dobind > 0) {
		if (bind (s, (struct sockaddr *) &lsa, sizeof (lsa)) < 0) {
			nlog (LOG_WARNING, LOG_CORE, "bind(): Warning, Couldn't bind to IP address %s", strerror (errno));
		}
	}

	bzero (&sa, sizeof (sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons (port);
	bcopy (hp->h_addr, (char *) &sa.sin_addr, hp->h_length);

	ret=connect (s, (struct sockaddr *) &sa, sizeof (sa));
	if (ret< 0) {
		close (s);
		return NS_FAILURE;
	}
	return s;
}

void client_activity(int fd, short eventtype, void *arg) {
	void *buf[BUFSIZE];
	mqclient *mqc = arg;
	int i, dorun;
	SET_SEGV_LOCATION();
	dorun = 1;
	if (eventtype == EV_READ) {
		while (dorun) {
			bzero(buf, BUFSIZE);
			i = read(fd, buf, BUFSIZE);
			if (i < 1) {
				/* error */
				nlog(LOG_WARNING, LOG_CORE, "Read Error on %d: %s", fd, strerror(errno));
				/* delete sock */
				event_del(&mqc->ev);
				del_client(mqc);
				close(fd);
				return;
			} else {
				buffer_add(mqc, buf, i);
				if (i <= BUFSIZE) {
					/* we read less then bufsize, so there is no more data */
					dorun = 0;
				}
			}
		}
	}

	/* only process the packet if we are ready */
	if (mqc->pck) {
		pck_parse_packet(mqc->pck, mqc->buffer, mqc->offset);
	}	
}



void listen_accept(int fd, short eventtype, void *arg) {
        unsigned int al;
        struct sockaddr_in client_address;
        int l;
	mqclient *mqc;
	SET_SEGV_LOCATION();
        memset (&client_address, 0, al = sizeof (client_address));
        l = accept (fd, (struct sockaddr *)&client_address, &al);
	if (l < 0) {
		nlog(LOG_WARNING, LOG_CORE, "Accept Failed on %d: %s", fd, strerror(errno));
		return;
	}
	mqc = new_client(l);
 	MQC_SET_STAT_CONNECT(mqc);
	MQC_SET_TYPE_CLIENT(mqc);
	mqc->ip = client_address;
	strncpy(mqc->host, inet_ntoa(client_address.sin_addr), MAXHOST);
	nlog(LOG_DEBUG1, LOG_CORE, "New Connection from %s on fd %d", mqc->host, mqc->fd);
	/* start DNS lookup */
	do_reverse_lookup(mqc);
	event_set(&mqc->ev, l, EV_READ|EV_PERSIST, client_activity, mqc);
	event_add(&mqc->ev, NULL);	
}



/** @brief main recv loop
 *
 * @param none
 * 
 * @return none
 */
void
start ()
{
	int servsock;
	struct event ev;
	
	SET_SEGV_LOCATION();
	servsock = listen_on_port(me.port);
	if (servsock < 0) {
		do_exit(NS_EXIT_ERROR, "Can't Create Client Port");
		return;
	}
	event_set(&ev, servsock, EV_READ|EV_PERSIST, listen_accept, NULL);
	event_add(&ev, NULL);	
	nlog(LOG_NOTICE, LOG_CORE, "Listening for new connections on port %d: (%d)", me.port, servsock);

	while (1) {
		SET_SEGV_LOCATION();

		setup_dns_socks();

		event_loop(EVLOOP_ONCE);
		if (me.die) {
			do_exit(NS_EXIT_NORMAL, "Normal Exit");
		}
	}

	do_exit(NS_EXIT_ERROR, "Exit from Loop");
}



/** @brief get max available sockets
 *
 * @param none
 * 
 * @return returns the max available socket 
 */
int
getmaxsock ()
{
	struct rlimit *lim;
	int ret;

	SET_SEGV_LOCATION();
	lim = malloc (sizeof (struct rlimit));
	getrlimit (RLIMIT_NOFILE, lim);
	ret = lim->rlim_max;
	free (lim);
	return ret;
}

/** @brief send to socket
 *
 * @param fmt printf style vaarg list of text to send
 * 
 * @return none
 */
void
sts (const char *buf, const int buflen)
{
	int sent;

	SET_SEGV_LOCATION();
	if (servsock == -1) {
		nlog(LOG_WARNING, LOG_CORE, "Not sending to server as we have a invalid socket");
		return;
	}
	sent = write (servsock, buf, buflen);
	if (sent == -1) {
		nlog (LOG_CRITICAL, LOG_CORE, "Write error: %s", strerror(errno));
		do_exit (NS_EXIT_ERROR, NULL);
	}
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
listen_on_port(int port)
{
  int      srvfd;      /* FD for our listen server socket */
  struct sockaddr_in srvskt;
  int      adrlen;
  int      flags;

  SET_SEGV_LOCATION();
  adrlen = sizeof(struct sockaddr_in);
  (void) memset((void *) &srvskt, 0, (size_t) adrlen);
  srvskt.sin_family = AF_INET;
  /* bind to the local IP */
  if (dobind) {
	srvskt.sin_addr = lsa.sin_addr;
  } else {
  	srvskt.sin_addr.s_addr = INADDR_ANY;
  }
  srvskt.sin_port = htons(me.port);
  if ((srvfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    nlog(LOG_CRITICAL,LOG_CORE, "SqlSrv: Unable to get socket for port %d.", port);
    return -1;
  }
  flags = fcntl(srvfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  (void) fcntl(srvfd, F_SETFL, flags);
//  setsockopt(srvfd, SOL_SOCKET, SO_REUSEADDR, (char*) 1, sizeof(1));

  if (bind(srvfd, (struct sockaddr *) &srvskt, adrlen) < 0)
  {
    nlog(LOG_CRITICAL, LOG_CORE, "Unable to bind to port %d", port);
    return -1;
  }
  if (listen(srvfd, 1) < 0)
  {
    nlog(LOG_CRITICAL, LOG_CORE, "Unable to listen on port %d", port);
    return -1;
  }
  return (srvfd);}



void setup_dns_socks() {
	/* acutally it would be better to use the select/poll interface, but this makes it easy */
	adns_processany(ads);
	do_dns();
}

int
buffer_add(mqclient *mqc, void *data, size_t datlen)
{
	size_t need = mqc->offset + datlen;
	size_t oldoff = mqc->offset;

	if (mqc->bufferlen < need) {
		void *newbuf;
		int length = mqc->bufferlen;

		if (length < 256)
			length = 256;
		while (length < need)
			length <<= 1;

		if ((newbuf = realloc(mqc->buffer, length)) == NULL)
			return (-1);
		mqc->buffer = newbuf;
		mqc->bufferlen = length;
	}

	memcpy(mqc->buffer + mqc->offset, data, datlen);
	mqc->offset += datlen;

#if 0
	if (datlen && mqc->cb != NULL)
		(*mqc->cb)(mqc, oldoff, mqc->off, mqc->cbarg);
#endif
	return (0);
}
