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
#include <pthread.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "list.h"
#include "dns.h"
#include "client.h"
#include "queuemanager.h"
#include "messagequeue.h"



void mq_new_client(messqitm *mqi) {
	mqclient *mqc;
	lnode_t *node;
	
	/* create the new client struct */
	mqc = malloc(sizeof(mqclient));
	MYLOCK_INIT(mqc->lock);
	strncpy(mqc->username, mqi->data.new_clnt.username, MAXUSER);
	strncpy(mqc->host, mqi->data.new_clnt.host, MAXHOST);
	mqc->clntid = mqi->conid;
	
	nlog(LOG_DEBUG1, LOG_CORE, "Created new MQ Client for %s@%s", mqc->username, mqc->host);

	node = lnode_create(mqc);
	MYLOCK(&mq_clientslock);
	/* lock the individual clients struct now as well */
	MYLOCK(&mqc->lock);
	list_append(mq_clients, node);
	MYUNLOCK(&mq_clientslock);



	/* before we return, unlock the client */
	MYUNLOCK(&mqc->lock);
}

static int compare_mqclient(const void *key1, const void *key2) {
	mqclient *mqc = (mqclient *)key1;
	int conid = (int)key2;
	MYLOCK(&mqc->lock);
	if (mqc->clntid == conid) {
		MYUNLOCK(&mqc->lock);
		return 0;
	} 
	MYUNLOCK(&mqc->lock);
	return -1;
}
	

static mqclient *find_client(int id) {
	lnode_t *node;
	MYLOCK(&mq_clientslock);
	node = list_find(mq_clients, (void *)id, compare_mqclient);
	MYUNLOCK(&mq_clientslock);
	if (node) {
		return lnode_get(node);
	}
	return NULL;
}		

void mq_del_client(messqitm *mqi) {
	mqclient *mqc;
	lnode_t *node;

	MYLOCK(&mq_clientslock);
	node = list_find(mq_clients, (void *)mqi->conid, compare_mqclient);
	if (node) {
		mqc = lnode_get(node);
		MYLOCK(&mqc->lock);
		nlog(LOG_DEBUG1, LOG_CORE, "Deleting %s out of MQ Clients", mqc->username);
		list_delete(mq_clients, node);
		/* finished with the list, unlock for other threads */
		MYUNLOCK(&mq_clientslock);
		lnode_destroy(node);
		MYUNLOCK(&mqc->lock);
		free(mqc);
	} else {
		nlog(LOG_DEBUG1, LOG_CORE, "Couldn't find Client ID %ld", mqi->conid);
	}	
}

