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
** $Id: client.c 15 2004-05-18 12:30:04Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
                     
#include "defines.h"
#include "conf.h"
#include "log.h"
#include "mythread.h"



int init_threadengine() {
	mythreads *tid;
	lnode_t *node;
	MYLOCK_INIT(mythreadengine);
	
	threads = list_create(-1);
	tid = malloc(sizeof(mythreads));
	snprintf(tid->name, BUFSIZE, "Main");
	tid->tid = pthread_self();
	node = lnode_create(tid);
	list_append(threads, node);
	return NS_SUCCESS;
}


void mylock_(mylocks *what, char *file, char *func) {
	long tid;
	tid = pthread_self();
#ifdef THREADDEBUG
	printf("TID:%s %s(%s): Trying to Lock %s\n", get_thread_name(tid), func, file, what->name);
#endif
	pthread_mutex_lock(&what->lock);
	snprintf(what->who, BUFSIZE, "%s-%s", file, func);
	what->thread = tid;
}

void myunlock_ (mylocks *what, char *file, char *func) {
	long tid;
	tid = pthread_self();
#ifdef THREADDEBUG
	printf("TID:%s %s(%s): Trying to Unlock %s\n", get_thread_name(tid), func, file, what->name);
#endif
	bzero(what->who, BUFSIZE);
	what->thread = 0;
	pthread_mutex_unlock(&what->lock);
}

#if 0
void lock_init(mylocks *what) {
	what->lock = PTHREAD_MUTEX_INITIALIZER;
}
#endif


int create_thread(char *name, void *(*start)(void *), void *arg) {
	pthread_t adminthread;
	pthread_attr_t attr;
	int rc;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rc = pthread_create(&adminthread, &attr, start, arg);
	if (rc < 0) {
		nlog(LOG_WARNING, LOG_CORE, "Can't Spawn New Thread: %s", strerror(errno));
		pthread_attr_destroy(&attr);
		return NS_FAILURE;
	}
	pthread_attr_destroy(&attr);
	return rc;
}

int thread_created(char *name) {
	mythreads *tid;
	lnode_t *node;

	tid = malloc(sizeof(mythreads));
	snprintf(tid->name, BUFSIZE, "%s", name);
	tid->tid = pthread_self();
	nlog(LOG_DEBUG1, LOG_CORE, "Spawning %s Thread: %ld", tid->name, tid->tid);
	node = lnode_create(tid);
	MYLOCK(&mythreadengine);
	list_append(threads, node);
	MYUNLOCK(&mythreadengine);
	return NS_SUCCESS;
}


static int
compare_tid (const void *key1, const void *key2)
{
        mythreads *tid = (void *) key1;
        long tidnum = (long) key2;
        if (tid->tid == tidnum) {
        	return 0;
        } else {
        	return -1;
        }
}

lnode_t * 
find_thread_node(long tid, list_t *queue) {
	return(list_find(threads, (void *) tid, compare_tid));
}

char *get_thread_name(long tidnum) {
	mythreads *tid;
	lnode_t *node;
//	MYLOCK(&mythreadengine);
	node = find_thread_node(tidnum, threads);
	if (node) {
		tid = lnode_get(node);
//		MYUNLOCK(&mythreadengine);
		return tid->name;
	} 
//	MYUNLOCK(&mythreadengine);
	return NULL;
}

int count_threads(char *name) {
	lnode_t *node;
	mythreads *tid;
	int i = 0;
	
	if (name) {
		node = list_first(threads);
		while (node) {
			tid = lnode_get(node);
			if (!strcasecmp(tid->name, name)) {
				i++;
			}
			node = list_next(threads, node);
		}
		return i;
	} else {
		return (int) list_count(threads);
	}
	return 0;
}

int destroy_thread() {
    	mythreads *tid;
    	lnode_t *node;


    	MYLOCK(&mythreadengine);
    	node = find_thread_node(pthread_self(), threads);
    	if (node) {
    		tid = lnode_get(node);
		nlog(LOG_DEBUG1, LOG_CORE, "%s Thread Ended: %ld", tid->name, tid->tid);
    		list_delete(threads, node);
    		lnode_destroy(node);
    		free(tid);
    	}
	MYUNLOCK(&mythreadengine);
    	pthread_exit(NULL);
    	return NS_SUCCESS;
}
