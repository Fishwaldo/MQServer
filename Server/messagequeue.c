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
** $Id: client.c 30 2004-09-26 13:54:02Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "list.h"
#include "serversock.h"
#include "queuemanager.h"
#include "mythread.h"
#include "tcprioq.h"
#include "messagequeue.h"
#include "dns.h"
#include "client.h"


void mq_join_queue(messqitm *mqi, myqueues *messq);
void mq_send_msg(messqitm *mqi, myqueues *messq);

void *init_messqueue(void *arg) {
	int ret;
	myqueues *messq = (myqueues *)arg;
	int candie = 0;
	messqitm *mqi;

	
	thread_created("messqm");
	
	/* setup */
	
	/* loop and wait */
	
	while (candie != 1) {
		/* XXX config a timeout */
		ret = qm_wait(messq, 60);
		if (ret == ETIMEDOUT) {
			pthread_mutex_unlock(&messq->mutex);
			/* say goodnight */
			candie = 1;		
		} else {
			pthread_mutex_unlock(&messq->mutex);
			tcprioq_get(messq->inqueue, (void *)&mqi);
printf("Thread %d\n", pthread_self());
			switch (mqi->type) {
				case MQI_TYPE_NEWCLNT:
printf("newclnt\n");
					mq_new_client(mqi);
					break;
				case MQI_TYPE_DELCLNT:
printf("delclnt\n");
					mq_del_client(mqi);
					break;
				case MQI_TYPE_JOINQ:
printf("joinq\n");
					mq_join_queue(mqi, messq);
					break;
				case MQI_TYPE_MES:
printf("msg\n");
					mq_send_msg(mqi, messq);
					break;
				default:
					nlog(LOG_WARNING, LOG_MESSQ, "MQ Unknown Message Type %d in MessageQueue, Discarding", mqi->type);
					break;
			}


			free(mqi);
#if 0
			/* Put it back in the outqueue */
			pthread_mutex_lock(&messq->mutex);
			tcprioq_add(messq->outqueue, (void *)mqi);
			pthread_mutex_unlock(&messq->mutex);
#endif
		}
	}
	/* finished */
	destroy_thread();
	return NULL;
}

mqqueue *mq_create_queue(char *name) {
	mqqueue *que;
	hnode_t *node;
	/* XXX Load queue defaults out of config/database? */

	que = malloc(sizeof(mqqueue));
	strncpy(que->name, name, MAXQUEUE);
	que->clients = hash_create(-1, 0, 0);
	que->nxtmsgid = 1;
	que->clntflags = 0;
	MYLOCK_INIT(que->lock);
	MYLOCK(&mq_queuelock);
	node = hnode_create(que);
	hash_insert(mq_queues, node, que->name);
	MYUNLOCK(&mq_queuelock);
	return que;
}

mqqueue *find_queue(char *name) {
	hnode_t *node;
	MYLOCK(&mq_queuelock);
	node = hash_lookup(mq_queues, name);
	MYUNLOCK(&mq_queuelock);
	if (node) {
		return hnode_get(node);
	} else {
		return NULL;
	}
}


void mq_join_queue(messqitm *mqi, myqueues *messq) {
	mqclient *cli;
	mqqueue *que;
	hnode_t *node, *node2;
	mqqueuemember *qm;
	messqitm *sndmqi;
	
	/* find the client */
	cli = find_client(mqi->conid);
	if (!cli) {
		nlog(LOG_WARNING, LOG_MESSQ, "MQ Can't Find Client %ld for joinqueue", mqi->conid);
		return;
	}
	/* lock the client */
	MYLOCK(&cli->lock);

	/* test to make sure the client isn't already a member of the queue */
	if (hash_lookup(cli->queues, mqi->data.joinqueue.queue)) {
		/* already a member, just drop silently */
		nlog(LOG_WARNING, LOG_MESSQ, "MQ Client %s@%s is already a member of queue %s", cli->username, cli->host, mqi->data.joinqueue.queue);
		MYUNLOCK(&cli->lock);
		return;
	}
	
	/* find the queue */
	que = find_queue(mqi->data.joinqueue.queue);
	if (!que) {
		que = mq_create_queue(mqi->data.joinqueue.queue);
	}
	MYLOCK(&que->lock);
	/* ok, now we have a queue set
	** add the client to the queue list and 
	** add the queue to the client 
	*/
	qm = malloc(sizeof(mqqueuemember));
	qm->cli = cli;
	strncpy(qm->filter, mqi->data.joinqueue.filter, BUFSIZE);
	qm->flags = mqi->data.joinqueue.flags;
	node2 = hnode_create(qm);
	hash_insert(que->clients, node2, cli->username);
	
	node = hnode_create(que);
	hash_insert(cli->queues, node, que->name);
	nlog(LOG_DEBUG1, LOG_MESSQ, "MQ Joined Client %s@%s to Queue %s", cli->username, cli->host, que->name);
	
	/* XXX send queueinfo to the client */
	sndmqi = malloc(sizeof(messqitm));
	sndmqi->type = MQI_SEND_QI;
	sndmqi->prio = PRIOQ_NORMAL;
	sndmqi->conid = mqi->conid;
	strncpy(sndmqi->data.joinqueue.queue, que->name, MAXQUEUE);

	/* XXX Filter and Flags */
	sndmqi->data.joinqueue.filter[0] = '\0';
	sndmqi->data.joinqueue.flags = que->clntflags;
	pthread_mutex_lock(&messq->mutex);
	nlog(LOG_DEBUG1, LOG_MESSQ, "MQ Sending Queueinfo to %s@%s for queue %s",cli->username, cli->host, que->name);
	tcprioq_add(messq->outqueue, (void *)sndmqi);
	pthread_mutex_unlock(&messq->mutex);
	/* unlock the locks */
	MYUNLOCK(&cli->lock);
	MYUNLOCK(&que->lock);
}

void mq_send_msg(messqitm *mqi, myqueues *messq) {
	mqclient *sendcli;
	mqqueue *que;
	hnode_t *node;
	mqqueuemember *sendqm, *rcptqm;
	hscan_t hs;
	messqitm *sndmqi;
	
	/* find the client */
	sendcli = find_client(mqi->conid);
	if (!sendcli) {
		nlog(LOG_WARNING, LOG_MESSQ, "MQ Can't Find Client %ld for sendmsg", mqi->conid);
		return;
	}
	/* lock the client */
	MYLOCK(&sendcli->lock);

	/* find the queue */
	que = find_queue(mqi->data.msg.queue);
	if (!que) {
		/* if the queue doesn't exist, drop the message */
		nlog(LOG_WARNING, LOG_MESSQ, "MQ Can't find Queue %s for client %s@%s message", mqi->data.msg.queue, sendcli->username, sendcli->host);
		MYUNLOCK(&sendcli->lock);
		/* XXX Send Error Message */
		return;
	}
	MYLOCK(&que->lock);


	/* get the queuemember entry for this user so we can see flags etc.*/
	node = hash_lookup(que->clients, sendcli->username);
	if (node) {
		sendqm = hnode_get(node);
	} else {
		sendqm = NULL;
	}	
	
	/* XXX is client authed to send to this queue */
	

	/* XXX is the message persistant? if so, then store it */

	/* ok, if we get here, its authed, send the message to the clients */
	hash_scan_begin(&hs, que->clients);
	while ((node = hash_scan_next(&hs))) {
		rcptqm = hnode_get(node);
		/* XXX check flags to see if this client wants it including copies of his own messages */
		
		/* XXX check topic and filter to see if the client wants it */
		
		/* ok, client wants it, send the message */
		sndmqi = malloc(sizeof(messqitm));
		sndmqi->type = MQI_SEND_MES;
		sndmqi->prio = PRIOQ_NORMAL;
		sndmqi->conid = rcptqm->cli->clntid;
		sndmqi->data.sndmsg.len = mqi->data.msg.len;
		sndmqi->data.sndmsg.messid = que->nxtmsgid++;
		sndmqi->data.sndmsg.timestamp = time(NULL);
		strncpy(sndmqi->data.sndmsg.from, sendcli->username, MAXUSER);
		strncpy(sndmqi->data.sndmsg.topic, mqi->data.msg.topic, MAXQUEUE);
		strncpy(sndmqi->data.sndmsg.queue, que->name, MAXQUEUE);
		sndmqi->data.sndmsg.msg = malloc(mqi->data.msg.len+1);
		strncpy(sndmqi->data.sndmsg.msg, mqi->data.msg.msg, mqi->data.msg.len);
		pthread_mutex_lock(&messq->mutex);
		nlog(LOG_DEBUG1, LOG_MESSQ, "MQ Sending Message from %s@%s to %s@%s on queue %s", sendcli->username, sendcli->host, rcptqm->cli->username, rcptqm->cli->host, que->name);
		tcprioq_add(messq->outqueue, (void *)sndmqi);
		pthread_mutex_unlock(&messq->mutex);
	}						
	MYUNLOCK(&que->lock);
	MYUNLOCK(&sendcli->lock);
	free(mqi->data.msg.msg);
}
