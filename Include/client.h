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

typedef struct mqclient {
	int fd;
	char user[MAXNICK];
	char pass[MAXPASS];
	char host[MAXHOST];
	struct sockaddr_in ip;
	long status;
	long type;
	struct event ev;
	lnode_t *node;
} mqclient;


#define MQC_STAT_CONNECT	0x01
#define MQC_STAT_DNSLKUP	0x02
#define MQC_STAT_AUTHSTART	0x04
#define MQC_STAT_AUTHOK		0x08

#define MQC_TYPE_NONE		0x01
#define MQC_TYPE_LISTEN		0x02
#define MQC_TYPE_CLIENT		0x04
#define MQC_TYPE_ADMIN		0x08
#define MQC_TYPE_REPL		0x10

#define MQC_SET_TYPE_LISTEN(x) x->type = MQC_TYPE_LISTEN
#define MQC_IS_TYPE_LISTEN(x) (x->type & MQC_TYPE_LISTEN)
#define MQC_CLEAR_TYPE_LISTEN(x) (x->type &= MQC_TYPE_LISTEN)

#define MQC_SET_STAT_CONNECT(x) x->status |= MQC_STAT_CONNECT
#define MQC_IS_STAT_CONNECT(x) (x->status & MQC_STAT_CONNECT)
#define MQC_CLEAR_STAT_CONNECT(x) (x->status &= MQC_STAT_CONNECT);

#define MQC_SET_STAT_DNSLOOKUP(x) x->status |= MQC_STAT_DNSLKUP
#define MQC_IS_STAT_DNSLOOKUP(x) (x->status & MQC_STAT_DNSLKUP)
#define MQC_CLEAR_STAT_DNSLOOKUP(x) (x->status &= MQC_STAT_DNSLKUP);


mqclient *new_client(int);
void del_client(mqclient *);
#endif
