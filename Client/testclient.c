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
** $Id: sock.c 4 2004-04-28 12:02:07Z Fish $
*/

#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
                     
#include "defines.h"
#include "packet.h"
#include "log.h"

void pck_logger(char *fmt,...);
int conid;

typedef struct testdata {
	char *name;
	int size;
	char testdata[255];
} testdata;

/* define for readstr */
void *readstr(void *data, size_t *size);


structentry testdataentry[] = {
	{
		STR_PSTR,	/* its a pointer to a string */
		0,		/* pointers are unknown, have to use a callback */
		0, 		/* for callbacks, no offset is required */
		readstr
	},
	{
		STR_INT,	/* its a integer */
		sizeof(int),	/* its the size of a int */
		offsetof(struct testdata, size), /* we need to know the offset for this one */
		NULL		/* no callback is required for simple ints */
	},
	{
		STR_STR,	/* its a string */
		255,		/* the string size */
		offsetof(struct testdata, testdata),
		NULL
	}
};
testdata *tmp;
	
					
void *readstr(void *data, size_t *size) {
	testdata *tmp = (testdata *)data;
	*size = strlen(tmp->name);
	return tmp->name;
}

int gotaction(int type, void *cbarg) {
	mq_data_joinqueue *qi;
	mq_data_senddata *sd;
	switch (type) {
		case PCK_SMP_LOGINOK:
			pck_simple_joinqueue(conid, "testqueue", 0, "*");
			pck_simple_send_message_struct(conid, &testdataentry, (sizeof(testdataentry)/sizeof(structentry)), tmp, "testqueue", "Mytopic");
			break;
		case PCK_SMP_QUEUEINFO:
			qi = pck_get_queueinfo(conid);
			printf("queue info: %s %d %s\n", qi->queue, qi->flags, qi->filter);
			break;
		case PCK_SMP_MSGFROMQUEUE:
			sd = pck_get_msgfromqueue(conid);
			printf("queue %s topic %s messid %d time %d from %s\n", sd->queue, sd->topic, sd->messid, sd->timestamp, sd->from);
	}			

}
int main() {
	int rc = 1;
	int ok = 0;
	
#if 0
	init_socket();
	debug_socket(1);
#endif
	tmp = malloc(sizeof(testdata));
	tmp->name = malloc(234);
	snprintf(tmp->name, 234, "hello world this is my testdata name");
	tmp->size = 5643;
	snprintf(tmp->testdata, 255, "and this is my static string");
	
	printf("total size %d\n", (sizeof(testdataentry)/sizeof(structentry)));
	
	
	conid = pck_make_connection("snoopy", "fish", "haha", 0, NULL, gotaction);
	while (rc == 1) {
		rc = pck_process();
		sleep(1);
	}
}

                                                

