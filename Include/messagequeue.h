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

#ifndef MESSAGEQUEUE_H
#define MESSAGEQUEUE_H
#include "client.h"

hash_t *mq_queues;
mylocks mq_queuelock;


typedef struct mqqueue {
	char name[MAXQUEUE];
	hash_t *clients;
	mylocks lock;
	long nxtmsgid;
	long clntflags;
} mqqueue;

typedef struct mqqueuemember {
	mqclient *cli;
	char filter[BUFSIZE];
	long flags;
} mqqueuemember;



void *init_messqueue(void *arg);
int setup_messq();

#endif
