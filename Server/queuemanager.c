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
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "list.h"
#include "serversock.h"
#include "queuemanager.h"
#include "authqueue.h"
#include "messagequeue.h"
#include "mythread.h"
#include "tcprioq.h"
#include "dns.h"
#include "callback.h"


myqueues *authq;
myqueues *messq;

int qm_cmp(const void *key1, const void *key2) {
	authqitm *aqi = (authqitm *)key1;
	authqitm *aqi2 = (authqitm *)key2;
	return (aqi2->prio - aqi->prio);
}	

extern int queueman_init() {
	/* this is the authorize message queue */
	authq = malloc(sizeof(myqueues));
	authq->inqueue = tcprioq_new(AUTHQSIZE, 1, qm_cmp);
	authq->outqueue = tcprioq_new(AUTHQSIZE, 1, qm_cmp);
	pthread_mutex_init(&authq->mutex, NULL);
	pthread_cond_init(&authq->cond, NULL); 
	
	/* this is the message queue queue */
	messq = malloc(sizeof(myqueues));
	messq->inqueue = tcprioq_new(MESSQSIZE, 1, qm_cmp);
	messq->outqueue = tcprioq_new(MESSQSIZE, 1, qm_cmp);
	pthread_mutex_init(&messq->mutex, NULL);
	pthread_cond_init(&messq->cond, NULL);

	return NS_SUCCESS;
}	


extern int qm_newauthqitm(mqsock *mqs, char *username, char *password, int mid) {
	authqitm *aqi;
	int i;
	/* copy the iteam over */
	aqi = malloc(sizeof(authqitm));
	aqi->prio = PRIOQ_NORMAL;
	aqi->conid = mqs->connectid;
	strncpy(aqi->username, username, MAXUSER);
	strncpy(aqi->password, password, MAXUSER);
	strncpy(aqi->host, mqs->host, MAXHOST);
	aqi->ip = mqs->ip;
	aqi->mid = mid;
		
	/* lock the authq */
	pthread_mutex_lock(&authq->mutex);
	/* add it to the queue */
	tcprioq_add(authq->inqueue, aqi);
	
	/* check if we have a authq thread running */
	i = count_threads("authqm");
	/* if there is no thread, or we have x items in the queue, and have not reached out authq max threads setting, 
	 * then start up a new thread 
	 */
	if ((i <= 0) || ((tcprioq_items(authq->inqueue) > me.authqthreshold) && (i < me.authqmaxthreads))) {
		/* start a new thread */
		create_thread("authqm", init_authqueue, authq);
	}	
	/* wake up a thread */
	pthread_mutex_unlock(&authq->mutex);
	pthread_cond_signal(&authq->cond);
	return NS_SUCCESS;
}	


extern int qm_wait(myqueues *qi, int tmout) {
	struct timeval now;
	struct timespec timeout;
                             
	pthread_mutex_lock(&qi->mutex);
	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + tmout;
	timeout.tv_nsec = now.tv_usec;
	/* if there is something in the queue, run now, else wait */
	if (tcprioq_items(qi->inqueue) > 0) {
		return 1;
	} 
	return (pthread_cond_timedwait(&qi->cond, &qi->mutex, &timeout));
}	                                                         

extern int qm_check_authq() {
	authqitm *aqi;
	int i;
	
	/* lock the authq */
	pthread_mutex_lock(&authq->mutex);
	i = tcprioq_items(authq->outqueue);
	if (i > 0) {
		while (!tcprioq_get(authq->outqueue, (void *)&aqi)) {
			MQS_Auth_Callback(aqi);
			free(aqi);
		}	
	}                                                                                                                                                                                                             
	pthread_mutex_unlock(&authq->mutex);
	return NS_SUCCESS;
}

extern int qm_check_messq() {
	messqitm *mqi;
	int i;
	
	/* lock the authq */
	pthread_mutex_lock(&messq->mutex);
	i = tcprioq_items(messq->outqueue);
	if (i > 0) {
		while (!tcprioq_get(messq->outqueue, (void *)&mqi)) {
			/* XXX do something */
			MQS_Mesq_Callback(mqi);
			free(mqi);
		}	
	}                                                                                                                                                                                                             
	pthread_mutex_unlock(&messq->mutex);
	return NS_SUCCESS;
}

static void qm_checkthreads(char *name, void *arg) {
	int i;
	/* check if we have a authq thread running */
	i = count_threads(name);
	/* if there is no thread, or we have x items in the queue, and have not reached out authq max threads setting, 
	 * then start up a new thread 
	 */
	if (i <= 0) {
		/* there are no threads running */
		printf("no threads\n");
		create_thread(name, init_messqueue, arg);
		/* XXX This is bad */
		sleep(1);
	} 
#if 0
	else ((tcprioq_items(messq->inqueue) > me.messqthreshold) && (i < me.messqmaxthreads))) {
		/* start a new thread */
		create_thread(name, init_messqueue, arg);
	}	
#endif
}


/* this function is called when a new client is authenticated
 * and we send it to the messagequeue for processing of stored messages and 
 * setup the message queue client information 
 */
extern int qm_newclnt(mqsock *mqs, authqitm *aqi) {
	messqitm *mqi;
	
	mqi = malloc(sizeof(messqitm));
	/* new clients go in with low priority */
	mqi->prio = PRIOQ_NORMAL;
	mqi->conid = mqs->connectid;
	mqi->type = MQI_TYPE_NEWCLNT;
	strncpy(mqi->data.new_clnt.username, aqi->username, MAXUSER);
	strncpy(mqi->data.new_clnt.host, aqi->host, MAXHOST);
	
	/* lock the messq */
	pthread_mutex_lock(&messq->mutex);
	qm_checkthreads("messqm", messq);
	/* add it to the queue */
	tcprioq_add(messq->inqueue, mqi);

	/* wake up a thread */
	pthread_mutex_unlock(&messq->mutex);
	pthread_cond_signal(&messq->cond);
	return NS_SUCCESS;

}

/* this function is called when a client is deleted */
extern int qm_delclnt(mqsock *mqs) {
	messqitm *mqi;
	
	mqi = malloc(sizeof(messqitm));
	mqi->prio = PRIOQ_SLOW;
	mqi->conid = mqs->connectid;
	mqi->type = MQI_TYPE_DELCLNT;

	/* lock the messq */
	pthread_mutex_lock(&messq->mutex);
	qm_checkthreads("messqm", messq);
	/* add it to the queue */
	tcprioq_add(messq->inqueue, mqi);
	
	/* wake up a thread */
	pthread_mutex_unlock(&messq->mutex);
	pthread_cond_signal(&messq->cond);
	return NS_SUCCESS;
}	


/* this function is called when a client is deleted */
extern int qm_joinq(mqsock *mqs, char *queue, long flags, char *filter) {
	messqitm *mqi;
	
	mqi = malloc(sizeof(messqitm));
	mqi->prio = PRIOQ_NORMAL;
	mqi->conid = mqs->connectid;
	mqi->type = MQI_TYPE_JOINQ;
	strncpy(mqi->data.joinqueue.queue, queue, MAXQUEUE);
	strncpy(mqi->data.joinqueue.filter, filter, BUFSIZE);
	mqi->data.joinqueue.flags = flags;

	/* lock the messq */
	pthread_mutex_lock(&messq->mutex);
	qm_checkthreads("messqm", messq);
	/* add it to the queue */
	tcprioq_add(messq->inqueue, mqi);
	
	/* wake up a thread */
	pthread_mutex_unlock(&messq->mutex);
	pthread_cond_signal(&messq->cond);
	return NS_SUCCESS;
}	

/* this function is called when a client is deleted */
extern int qm_sendmsg(mqsock *mqs, char *queue, void *msg, size_t len, char *topic) {
	messqitm *mqi;
	
	mqi = malloc(sizeof(messqitm));
	mqi->prio = PRIOQ_NORMAL;
	mqi->conid = mqs->connectid;
	mqi->type = MQI_TYPE_MES;
	/* data is always base64 encoded */
	mqi->data.msg.msg = malloc(len+1);
	strncpy(mqi->data.msg.msg, msg, len);
	strncpy(mqi->data.msg.queue, queue, MAXQUEUE);
	strncpy(mqi->data.msg.topic, topic, MAXQUEUE);
	mqi->data.msg.len = len;

	/* lock the messq */
	pthread_mutex_lock(&messq->mutex);
	qm_checkthreads("messqm", messq);
	/* add it to the queue */
	tcprioq_add(messq->inqueue, mqi);
	
	pthread_mutex_unlock(&messq->mutex);
	pthread_cond_signal(&messq->cond);
	return NS_SUCCESS;
}	
