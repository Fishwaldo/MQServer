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
#include "authqueue.h"
#include "dns.h"

void *init_authqueue(void *arg) {
	int ret;
	myqueues *authq = (myqueues *)arg;
	int candie = 0;
	authqitm *aqi;

	
	thread_created("authqm");
	
	/* setup */
	
	/* loop and wait */
	
	while (candie != 1) {
		ret = qm_wait(authq, me.authqtimeout);
		if (ret == ETIMEDOUT) {
			pthread_mutex_unlock(&authq->mutex);
			/* say goodnight */
			candie = 1;		
		} else {
			tcprioq_get(authq->inqueue, (void *)&aqi);
			pthread_mutex_unlock(&authq->mutex);
			/* XXX Do auth */
			nlog(LOG_DEBUG1, LOG_CORE, "Auth %ld -  %s@%s (%s)\n", aqi->conid, aqi->username, aqi->host, aqi->password);
			aqi->result = NS_SUCCESS;

			/* put result in outqueu */
			pthread_mutex_lock(&authq->mutex);			
			tcprioq_add(authq->outqueue, (void *)aqi);
			pthread_mutex_unlock(&authq->mutex);
		}
	}
	/* finished */
	destroy_thread();
	return NULL;
}
