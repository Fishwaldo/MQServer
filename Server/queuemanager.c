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
#include "mythread.h"
#include "tcprioq.h"
#include "dns.h"


myqueues *authq;

int authqcmp(const void *key1, const void *key2) {
	authqitm *aqi = (authqitm *)key1;
	authqitm *aqi2 = (authqitm *)key2;
	return (aqi2->prio - aqi->prio);
}	

extern int queueman_init() {
	authq = malloc(sizeof(myqueues));
	authq->inqueue = tcprioq_new(AUTHQSIZE, 1, authqcmp);
	authq->outqueue = tcprioq_new(AUTHQSIZE, 1, authqcmp);
	pthread_mutex_init(&authq->mutex, NULL);
	pthread_cond_init(&authq->cond, NULL); 
	return NS_SUCCESS;
}	


extern int newauthqitm(mqsock *mqs, char *username, char *password, int mid) {
	authqitm *aqi;
	int i;
	
	/* lock the authq */
	pthread_mutex_lock(&authq->mutex);


	/* copy the iteam over */
	aqi = malloc(sizeof(authqitm));
	aqi->prio = PRIOQ_NORMAL;
	aqi->conid = mqs->connectid;
	strncpy(aqi->username, username, MAXUSER);
	strncpy(aqi->password, password, MAXUSER);
	strncpy(aqi->host, mqs->host, MAXHOST);
	aqi->ip = mqs->ip;
	aqi->mid = mid;
		
	/* add it to the queue */
	tcprioq_add(authq->inqueue, aqi);
	
	/* check if we have a authq thread running */
	i = count_threads("authqm");
	/* if there is no thread, or we have x items in the queue, and have not reached out authq max threads setting, 
	 * then start up a new thread 
	 */
	pthread_mutex_unlock(&authq->mutex);
	if ((i <= 0) || ((tcprioq_items(authq->inqueue) > me.authqthreshold) && (i < me.authqmaxthreads))) {
		/* start a new thread */
		create_thread("authqm", init_authqueue, authq);
	}	
	/* wake up a thread */
	pthread_cond_signal(&authq->cond);
	return NS_SUCCESS;
}	


extern int qm_wait(myqueues *qi, int tmout) {
	struct timeval now;
	struct timespec timeout;
	int retcode;
                             
	pthread_mutex_lock(&qi->mutex);
	gettimeofday(&now, NULL);
	timeout.tv_sec = now.tv_sec + tmout;
	timeout.tv_nsec = now.tv_usec;
	retcode = 0;
	return (pthread_cond_timedwait(&qi->cond, &qi->mutex, &timeout));
}	                                                         

extern int check_authq() {
	authqitm *aqi;
	int i;
	
	/* lock the authq */
	pthread_mutex_lock(&authq->mutex);
	i = tcprioq_items(authq->outqueue);
	if (i > 0) {
printf("checking authoutq %i\n", i);
		while (!tcprioq_get(authq->outqueue, (void *)&aqi)) {
printf("%ld %d\n", aqi->conid, aqi->mid);
			MQS_Auth_Callback(aqi->conid, aqi->result, aqi->mid);
			free(aqi);
		}	
	}                                                                                                                                                                                                             
	pthread_mutex_unlock(&authq->mutex);
}