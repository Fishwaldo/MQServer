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
#include "adns.h"
#include "mythread.h"
#include "queuemanager.h"

list_t *mq_clients;
mylocks mq_clientslock;



typedef struct mqclient {
	int clntid;
	mylocks lock;
	char username[MAXNICK];
	char host[MAXHOST];
	hash_t *queues;
} mqclient;

void mq_new_client(messqitm *mqi);
void mq_del_client(messqitm *mqi);
extern mqclient *find_client(int id);


#endif
