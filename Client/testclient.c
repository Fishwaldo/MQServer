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
#include <stdio.h>
#define READLINE_CALLBACKS 1;
#include <readline/readline.h>
#include <readline/history.h>
#include <ncurses.h>
#include <term.h>
                     
#include "defines.h"
#include "packet.h"
#include "log.h"

void pck_logger(char *fmt,...);
int conid;
int candie;

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
		offsetof(struct testdata, name), /* for callbacks, no offset is required */
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
testdata *tmp2;
int queueok;
WINDOW *dbgwin;
WINDOW *msgwin;
WINDOW *sndwin;
	
					
void *readstr(void *data, size_t *size) {
	testdata *tmp = (testdata *)data;
	*size = strlen(tmp->name);
	return tmp->name;
}


void readlinecallback(char *string) {
	if (!strcasecmp(string, "quit")) {
		endwin();
		exit(1);
	}
	printf("send\n");
	snprintf(tmp->name, 512, "%s", string);
	pck_simple_send_message_struct(conid, &testdataentry, (sizeof(testdataentry)/sizeof(structentry)), tmp, "testqueue", "MyTopic");
}


int gotaction(int type, void *cbarg) {
	mq_data_joinqueue *qi;
	mq_data_senddata *sd;
	char prompt[512];
	switch (type) {
		case PCK_SMP_LOGINOK:
			printf("Login Accepted\n");
			pck_simple_joinqueue(conid, "testqueue", 0, "*");
			break;
		case PCK_SMP_QUEUEINFO:
			qi = pck_get_queueinfo(conid);
			printf("Queue info: %s %ld %s\n", qi->queue, qi->flags, qi->filter);
			pck_simple_send_message_struct(conid, &testdataentry, (sizeof(testdataentry)/sizeof(structentry)), tmp, "testqueue", "Mytopic");
			snprintf(prompt, 512, "Queue %s>", qi->queue);
			rl_callback_handler_install(prompt, readlinecallback);
			queueok = 1;
			break;
		case PCK_SMP_MSGFROMQUEUE:
			sd = pck_get_msgfromqueue(conid);
			tmp2 = malloc(sizeof(testdata));
			pck_decode_message(sd, testdataentry,(sizeof(testdataentry)/sizeof(structentry)), tmp2);
			printf("Msg: Queue: %s topic %s messid %d time %d from %s\n", sd->queue, sd->topic, sd->messid, sd->timestamp, sd->from);
			printf("Msg: %s %ul %s\n", tmp2->name, tmp2->size, tmp2->testdata);
			tmp->size++;
/* 			pck_simple_send_message_struct(conid, &testdataentry, (sizeof(testdataentry)/sizeof(structentry)), tmp, "testqueue", "Mytopic"); */
			free(tmp2->name);
			free(tmp2);
			if (tmp->size == -1) {
				candie = 1; 
			}
	}			
	return 0;
}
int main() {
	int rc = 1;
	int ok = 0;
	
         fd_set rfds;
         struct timeval tv;
         int retval;
         queueok =0;                       
         /* Watch stdin (fd 0) to see when it has input. */
         FD_ZERO(&rfds);
         FD_SET(0, &rfds);
         /* Wait up to one seconds. */
         tv.tv_sec = 0;
         tv.tv_usec = 0;
#if 0
         initscr();
//	cbreak();
	keypad(stdscr, TRUE);

	dbgwin = newwin(10, 80, 0, 0);
	msgwin = newwin(10, 80, 10, 0);
	sndwin = newwin(10, 80, 20, 0);
	box(dbgwin, 0, 0);
	box(msgwin, 0, 0);
	box(sndwin, 0, 0);
	wrefresh(dbgwin);
	wrefresh(msgwin);
	wrefresh(sndwin);
//	refresh();
//	printw("Press F1 to exit");
#endif
                                                                                          
#if 0
	init_socket();
	debug_socket(1);
#endif
	tmp = malloc(sizeof(testdata));
	tmp->name = malloc(234);
	snprintf(tmp->name, 234, "hello world this is my testdata name");
	tmp->size = 1;
	snprintf(tmp->testdata, 255, "and this is my static string");
	
	printf("total size %d\n", (sizeof(testdataentry)/sizeof(structentry)));
	
	
	candie = 0;
	printf("Connecting.....");
	conid = pck_make_connection("snoopy", "fish", "haha", 0, NULL, gotaction);
	printf("Done\n");
	while (rc == 1) {
		rc = pck_process();
#if 0
		touchwin(stdscr);
		touchwin(dbgwin);
		touchwin(msgwin);
		touchwin(sndwin);
		wrefresh(dbgwin);
		wrefresh(msgwin);
		wrefresh(sndwin);
//		refresh();
#endif
		if (queueok == 1) {
		         FD_ZERO(&rfds);
		         FD_SET(0, &rfds);
	                  retval = select(1, &rfds, NULL, NULL, &tv);
		         if (retval == -1) {
	         		perror("select()");
			} else {
				if (FD_ISSET(0, &rfds)) {
					rl_callback_read_char();
				}
			}
		}
		if (candie == 1)
			break;
	}
	pck_fini();
	free(tmp->name);
	free(tmp);
	endwin();
}

                                                

void fini() {
	pck_fini();
	free(tmp);
	exit(1);
}
