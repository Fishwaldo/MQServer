/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond
** http://www.neostats.net/
**
**  Portions Copyright (c) 2000-2001 ^Enigma^
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
** $Id: client.h 23 2004-07-27 10:57:56Z Fish $
*/

#ifndef CLIENT_H
#define CLIENT_H

#include "defines.h"
#include "event.h"
#include "list.h"
#include "packet.h"


typedef struct mqss {
	mqp *mqplib;
	list_t *connections;
	int xdrport;
	int xmlport;
	int adminport;
	unsigned long nxtconid;
} mqss;

mqss mqssetup;
	
typedef struct mqsock {
	mqpacket *mqp;
	char host[MAXHOST];
	unsigned long connectid;
	struct sockaddr_in ip;
	long status;
	long type;
	struct event ev;
	lnode_t *node;
	union {
		int fd;
	} data;	
} mqsock;

/* flags to indicate the connection status */
#define MQC_STAT_CONNECT	0x01
#define MQC_STAT_DNSLKUP	0x02
#define MQC_STAT_AUTHSTART	0x04
#define MQC_STAT_AUTHOK		0x08
#define MQC_STAT_OK		0x10

#define MQC_SET_STAT_CONNECT(x) x->status |= MQC_STAT_CONNECT
#define MQC_IS_STAT_CONNECT(x) (x->status & MQC_STAT_CONNECT)
#define MQC_CLEAR_STAT_CONNECT(x) (x->status &= MQC_STAT_CONNECT);

#define MQC_SET_STAT_DNSLOOKUP(x) x->status |= MQC_STAT_DNSLKUP
#define MQC_IS_STAT_DNSLOOKUP(x) (x->status & MQC_STAT_DNSLKUP)
#define MQC_CLEAR_STAT_DNSLOOKUP(x) (x->status &= MQC_STAT_DNSLKUP);

#define MQC_SET_STAT_AUTHSTART(x) x->status |= MQC_STAT_AUTHSTART
#define MQC_IS_STAT_AUTHSTART(x) (x->status & MQC_STAT_AUTHSTART)
#define MQC_CLEAR_STAT_AUTHSTART(x) (x->status &= MQC_STAT_AUTHSTART);

#define MQC_SET_STAT_AUTHOK(x) x->status |= MQC_STAT_AUTHOK
#define MQC_IS_STAT_AUTHOK(x) (x->status & MQC_STAT_AUTHOK)
#define MQC_CLEAR_STAT_AUTHOK(x) (x->status &= MQC_STAT_AUTHOK);

#define MQC_SET_STAT_OK(x) x->status |= MQC_STAT_OK
#define MQC_IS_STAT_OK(x) (x->status & MQC_STAT_OK)
#define MQC_CLEAR_STAT_OK(x) (x->status &= MQC_STAT_OK);


/* flags to indicate what type of connection this is */
#define MQC_TYPE_NONE		0x01
#define MQC_TYPE_LISTEN_XDR	0x02
#define MQC_TYPE_LISTEN_XML	0x04
#define MQC_TYPE_CLIENT_XDR	0x08
#define MQC_TYPE_CLIENT_XML	0x10
#define MQC_TYPE_LISTEN_ADMIN	0x20
#define MQC_TYPE_ADMIN		0x40
#define MQC_TYPE_REPL		0x80

#define MQC_SET_TYPE_LISTEN_XDR(x) x->type = MQC_TYPE_LISTEN_XDR
#define MQC_IS_TYPE_LISTEN_XDR(x) (x->type & MQC_TYPE_LISTEN_XDR)
#define MQC_CLEAR_TYPE_LISTEN_XDR(x) (x->type &= MQC_TYPE_LISTEN_XDR)

#define MQC_SET_TYPE_LISTEN_XML(x) x->type = MQC_TYPE_LISTEN_XML
#define MQC_IS_TYPE_LISTEN_XML(x) (x->type & MQC_TYPE_LISTEN_XML)
#define MQC_CLEAR_TYPE_LISTEN_XML(x) (x->type &= MQC_TYPE_LISTEN_XML)

#define MQC_SET_TYPE_NONE(x) x->type = MQC_TYPE_NONE
#define MQC_IS_TYPE_NONE(x) (x->type & MQC_TYPE_NONE)
#define MQC_CLEAR_TYPE_NONE(x) (x->type &= MQC_TYPE_NONE)

#define MQC_SET_TYPE_CLIENT_XDR(x) x->type = MQC_TYPE_CLIENT_XDR
#define MQC_IS_TYPE_CLIENT_XDR(x) (x->type & MQC_TYPE_CLIENT_XDR)
#define MQC_CLEAR_TYPE_CLIENT_XDR(x) (x->type &= MQC_TYPE_CLIENT_XDR)

#define MQC_SET_TYPE_CLIENT_XML(x) x->type = MQC_TYPE_CLIENT_XML
#define MQC_IS_TYPE_CLIENT_XML(x) (x->type & MQC_TYPE_CLIENT_XML)
#define MQC_CLEAR_TYPE_CLIENT_XML(x) (x->type &= MQC_TYPE_CLIENT_XML)

#define MQC_SET_TYPE_ADMIN(x) x->type = MQC_TYPE_ADMIN
#define MQC_IS_TYPE_ADMIN(x) (x->type & MQC_TYPE_ADMIN)
#define MQC_CLEAR_TYPE_ADMIN(x) (x->type &= MQC_TYPE_ADMIN)

#define MQC_SET_TYPE_LISTEN_ADMIN(x) x->type = MQC_TYPE_LISTEN_ADMIN
#define MQC_IS_TYPE_LISTEN_ADMIN(x) (x->type & MQC_TYPE_LISTEN_ADMIN)
#define MQC_CLEAR_TYPE_LISTEN_ADMIN(x) (x->type &= MQC_TYPE_LISTEN_ADMIN)


#define MQC_SET_TYPE_REPL(x) x->type = MQC_TYPE_REPL
#define MQC_IS_TYPE_REPL(x) (x->type & MQC_TYPE_REPL)
#define MQC_CLEAR_TYPE_REPL(x) (x->type &= MQC_TYPE_REPL)


void MQS_sock_start ();
extern mqsock *find_con_by_id(int id);

#endif
