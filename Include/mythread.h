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
** $Id: client.h 23 2004-07-27 10:57:56Z Fish $
*/

#ifndef MYTHREAD_H
#define MYTHREAD_H
#include "config.h"
#include "defines.h"
#include "list.h"


list_t *threads;



typedef struct locks {
	pthread_mutex_t lock;        
        char who[BUFSIZE];         
        long thread;
	char name[BUFSIZE];
} mylocks;

typedef struct mythreads {
	long tid;
	char name[BUFSIZE];
} mythreads;	

mylocks mythreadengine;


#define MYLOCK(x) mylock_(x, __FILE__, __FUNCTION__)
#define MYUNLOCK(x) myunlock_(x, __FILE__, __FUNCTION__)
#define MYLOCK_INIT(x) pthread_mutex_init(&x.lock, NULL)

void mylock_(mylocks *, char *, char *);
void myunlock_(mylocks *, char *, char *);

char *get_thread_name(long tid);
int destroy_thread();
int create_thread(char *name, void *(*start)(void *), void *arg);
int thread_created(char *name);
int init_threadengine();

#endif
