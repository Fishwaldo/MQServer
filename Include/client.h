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
** $Id$
*/

#ifndef CLIENT_H
#define CLIENT_H

#include "defines.h"
#include "event.h"
#include "list.h"
#include "packet.h"

typedef struct mqclient {
	int fd;
	char user[MAXNICK];
	char pass[MAXPASS];
	char host[MAXHOST];
	struct sockaddr_in ip;
	void *buffer;
	size_t bufferlen;
	size_t offset;
	long status;
	long type;
	struct event ev;
	lnode_t *node;
	mqprotocol *pck;
} mqclient;

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
#define MQC_TYPE_LISTEN		0x02
#define MQC_TYPE_CLIENT		0x04
#define MQC_TYPE_ADMIN		0x08
#define MQC_TYPE_REPL		0x10

#define MQC_SET_TYPE_LISTEN(x) x->type = MQC_TYPE_LISTEN
#define MQC_IS_TYPE_LISTEN(x) (x->type & MQC_TYPE_LISTEN)
#define MQC_CLEAR_TYPE_LISTEN(x) (x->type &= MQC_TYPE_LISTEN)

#define MQC_SET_TYPE_NONE(x) x->type = MQC_TYPE_NONE
#define MQC_IS_TYPE_NONE(x) (x->type & MQC_TYPE_NONE)
#define MQC_CLEAR_TYPE_NONE(x) (x->type &= MQC_TYPE_NONE)

#define MQC_SET_TYPE_CLIENT(x) x->type = MQC_TYPE_CLIENT
#define MQC_IS_TYPE_CLIENT(x) (x->type & MQC_TYPE_CLIENT)
#define MQC_CLEAR_TYPE_CLIENT(x) (x->type &= MQC_TYPE_CLIENT)

#define MQC_SET_TYPE_ADMIN(x) x->type = MQC_TYPE_ADMIN
#define MQC_IS_TYPE_ADMIN(x) (x->type & MQC_TYPE_ADMIN)
#define MQC_CLEAR_TYPE_ADMIN(x) (x->type &= MQC_TYPE_ADMIN)

#define MQC_SET_TYPE_REPL(x) x->type = MQC_TYPE_REPL
#define MQC_IS_TYPE_REPL(x) (x->type & MQC_TYPE_REPL)
#define MQC_CLEAR_TYPE_REPL(x) (x->type &= MQC_TYPE_REPL)


mqclient *new_client(int);
void del_client(mqclient *);
void buffer_del(mqclient *mqc, size_t drainlen);

#endif
