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

#ifndef QUEUEMANAGER_H
#define QUEUEMANAGER_H
#include "tcprioq.h"

#define AUTHQSIZE    20
#define MESSQSIZE    100


#define PRIOQ_SLOW   0
#define PRIOQ_NORMAL 1
#define PRIOQ_URGENT 2
#define PRIOQ_EMERG  3

typedef struct myqueues {
	tcprioq_t *inqueue;
	tcprioq_t *outqueue;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} myqueues;


typedef struct authqitm {
	int prio;
	unsigned long conid;
	char username[MAXUSER];
	char password[MAXUSER];
	char host[MAXHOST];
	struct sockaddr_in ip;
	int result;
	int mid;
} authqitm;


#define MQI_TYPE_NEWCLNT 1
#define MQI_TYPE_DELCLNT 2

typedef struct messqitm {
	int prio;
	unsigned long conid;
	int type;
	union {
		struct new_clnt {
			char username[MAXUSER];
			char host[MAXHOST];
		} new_clnt;
	} data;
} messqitm;



extern int queueman_init();
extern int newauthqitm(mqsock *mqs, char *username, char *password, int mid);
extern int check_authq();
extern int check_messq();
extern int qm_wait(myqueues *qi, int tmout);
#endif
