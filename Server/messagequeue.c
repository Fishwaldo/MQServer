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
#include "mythread.h"
#include "tcprioq.h"
#include "messagequeue.h"
#include "dns.h"

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
			/* XXX do something */



			/* Put it back in the outqueue */
			pthread_mutex_lock(&messq->mutex);
			tcprioq_add(messq->outqueue, (void *)mqi);
			pthread_mutex_unlock(&messq->mutex);
		}
	}
	/* finished */
	destroy_thread();
	return NULL;
}
