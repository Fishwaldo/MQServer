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
** $Id$
*/

#ifndef SOCK_H
#define SOCK_H

#define MQS_S_FLAG_GOTSRVCAP 	0x01
#define MQS_S_FLAG_SENTAUTH	0x02
#define MQS_S_FLAG_CONNECTOK	0x04


#define MQS_S_FLAG_SET_GOTSRVCAP(x) (x->flags |= MQS_S_FLAG_GOTSRVCAP)
#define MQS_S_FLAG_IS_GOTSRVCAP(x) (x->flags & MQS_S_FLAG_GOTSRVCAP)

#define MQS_S_FLAG_SET_SENTAUTH(x) (x->flags = MQS_S_FLAG_SENTAUTH)
#define MQS_S_FLAG_IS_SENTAUTH(x) (x->flags & MQS_S_FLAG_SENTAUTH)

#define MQS_S_FLAG_SET_CONNECTOK(x) (x->flags = MQS_S_FLAG_CONNECTOK)
#define MQS_S_FLAG_IS_CONNECTOK(x) (x->flags & MQS_S_FLAG_CONNECTOK)


/* these are the simple callback types */
#define PCK_SMP_LOGINOK		1
#define PCK_SMP_QUEUEINFO	2
#define PCK_SMP_MSGFROMQUEUE	3


#define PCK_SMP_CLNTCAPREJ	-1
#define PCK_SMP_AUTHREJ		-2


/* defines for the structentry type fields */
#define STR_PSTR	1
#define STR_STR		2
#define STR_INT		3






/* this is the standalone un-threadsafe interface */
typedef int (actioncbfunc)(int, void *);
int init_socket(actioncbfunc *);
int debug_socket(int i);
int enable_server(int port);
int pck_process ();
int pck_make_connection (char *hostname, char *, char *, long , void *cbarg, actioncbfunc *);
unsigned long pck_simple_send_message_struct(int conid, structentry *mystruct, int cols, void *data, char *destination, char *topic);
unsigned long pck_simple_joinqueue(int conid, char *queue, int flags, char *filter);
mq_data_joinqueue *pck_get_queueinfo(int conid);
mq_data_senddata *pck_get_msgfromqueue(int conid);
int pck_decode_message(mq_data_senddata *sd, structentry *mystruct, int cols, void *target);


#endif
