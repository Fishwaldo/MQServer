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
** $Id: client.h 3 2004-04-27 14:19:24Z Fish $
*/

#ifndef PACKET_H
#define PACKET_H

#include "defines.h"
#include "xds.h"
#include "xds_engine_xdr_mqs.h"

/* encoding engine structs */
#define NUMENGINES 40

#define ENG_TYPE_XDR 1
#define ENG_TYPE_XML 2

/* server or client or repl flags mqprotocol->servorclnt */
#define PCK_IS_CLIENT	0x1
#define PCK_IS_SERVER	0x2
#define PCK_IS_REPL	0x4

#define MQP_IS_CLIENT(x) (x->servorclnt & PCK_IS_CLIENT)
#define MQP_IS_SERVER(x) (x->servorclnt & PCK_IS_SERVER)
#define MQP_IS_REPL(x) (x->servorclnt & PCK_IS_REPL)

/* flags to indicate the message type */
#define PCK_ACK			1
#define PCK_ERROR		2
#define PCK_SRVCAP		3
#define PCK_AUTH		4
#define PCK_CLNTCAP		5
#define PCK_APPID		6
#define PCK_NOTIFY		7
#define PCK_FINDQUEUE		8
#define PCK_PRESENCE		9
#define PCK_JOINQUEUE		10
#define PCK_PARTQUEUE		11
#define PCK_CREATEQUEUE 	12
#define PCK_QUEUEINFO		13
#define PCK_ALTERQUEUE		14
#define PCK_SENDTOQUEUE		15
#define PCK_GETFROMQUEUE 	16
#define PCK_MSGFROMQUEUE	17
#define PCK_SENTTOCLNT		18
#define PCK_MSGFROMCLNT		19

/* format strings */
#define PCK_ACK_FMT		"int32 "
#define PCK_ERROR_FMT		"string"
#define PCK_SRVCAP_FMT		"int32 int32 string"
#define PCK_CLNTCAP_FMT		"int32 int32 string"
#define PCK_AUTH_FMT		"string string"

#define PCK_JOINQUEUE_FMT	"string int32 string"
#define PCK_SENDTOQUEUE_FMT	"string int32 string octet"
#define PCK_QUEUEINFO_FMT	"string int32 string"
#define PCK_MSGFROMQUEUE_FMT	"string string octet int32 int32 string"

/* packet flags, not message flags */
#define PCK_FLG_REQUIREACK		0x01
#define PCK_FLG_REQUIREACPPROCESS 	0x02


typedef struct mq_data_stream {
	char *queue;
	void *data;
	size_t datalen;
	size_t len;
	char *topic;
} mq_data_stream;

typedef struct mq_data_srvcap {
	int srvcap1;
	int srvcap2;
	char *capstr;
} mq_data_srvcap;

typedef struct mq_data_auth {
	char *username;
	char *password;
	char host[BUFSIZE];
	long flags;
} mq_data_auth;

typedef struct mq_data_joinqueue {
	char *queue;
	long flags;
	char *filter;
} mq_data_joinqueue;

typedef struct mq_data_senddata {
	char *queue;
	char *topic;
	void *data;
	size_t len;
	unsigned long messid;
	long timestamp;
	char *from;
} mq_data_senddata;

struct message {
	int MID;
	int MSGTYPE;
	int VERSION;
	int flags;
	union {
		struct mq_data_stream stream;
		struct mq_data_srvcap srvcap;
		struct mq_data_auth auth;
		struct mq_data_joinqueue joinqueue;
		struct mq_data_senddata sendmsg;
		char *string;
		int num;
	} data;
} message;	

/* client protocol struct */
typedef struct mqpacket {
	int nxtoutmid;
	int servorclnt;
	int sock;
	int pollopts;
	int flags;
	int wiretype;
	struct mq_data_auth si;
	void *cbarg;
	void *buffer;
	size_t bufferlen;
	size_t offset;
	void *outbuffer;
	size_t outbufferlen;
	size_t outoffset;
	struct message inmsg;
	struct message outmsg;
	xds_t *xdsin;
	xds_t *xdsout;
} mqpacket;	

typedef void (logfunc)(char *fmt, ...)  __attribute__((format(printf,1,2))); /* 3=format 4=params */
typedef int (connectauthfunc)(void *, mqpacket *);
typedef int (callbackfunc)(void *, mqpacket *);


typedef struct myeng {
	xds_mode_t dir;
	int type;
	xds_engine_t ptr;
	const char myname[BUFSIZE];
} myeng;


typedef struct mqp {
	logfunc *logger;
	connectauthfunc *connectauth;
	myeng *myengines;
	callbackfunc *callback;
} mqp; 	 

/* defines for the structentry type fields */
#define STR_PSTR	1
#define STR_STR		2
#define STR_INT		3


typedef struct {
	int type;
	size_t size;
	size_t offset;
	void *(*readcb) (void *data, size_t *size);
} structentry;	



extern int encode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args);
extern int decode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args);




void pck_set_logger(mqp *, logfunc *logger);
void pck_set_callback(mqp *, callbackfunc *);
void pck_set_authcallback(mqp *, connectauthfunc *);
void pck_set_data(mqpacket *, void *);
mqp *init_mqlib ();
int read_fd (mqp *mqplib, mqpacket *mqp);
int close_fd (mqp *mqplib, mqpacket * mqp);
int write_fd (mqp *mqplib, mqpacket * mqp);
mqpacket * pck_new_connection (mqp *mqplib, int fd, int type, int contype);
void print_decode(mqpacket *, int what);
int pck_parse_packet (mqp *mqplib, mqpacket * mqp, u_char * buffer, unsigned long buflen);
unsigned long pck_send_ack(mqp *mqplib, mqpacket *mqp, int MID);
unsigned long pck_send_error(mqp *mqplib, mqpacket *mqp, char *fmt, ...);
unsigned long pck_send_message_struct(mqp *mqplib, mqpacket *mqp, structentry *mystruct, int cols, void *data, char *destination, char *topic);
unsigned long pck_send_joinqueue(mqp *mqplib, mqpacket *mqpck, char *queue, int flags, char *filter);
unsigned long pck_send_srvcap(mqp *mqplib, mqpacket *mqp);
unsigned long pck_send_auth(mqp *mqplib, mqpacket *mqp, char *username, char *password);
unsigned long pck_send_clntcap(mqp *mqplib, mqpacket *mqp);
unsigned long pck_send_queueinfo(mqp *mqplib, mqpacket *mqpck, char *queue, char *info, int flags);
unsigned long pck_send_queue_mes(mqp *mqplib, mqpacket *mqpck, char *queue, char *topic, void *data, size_t len, unsigned long messid, long timestamp, char *from);
xds_t * pck_init_engines (mqp *mqplib, int type, int direction);

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

/* these are error defines */
#define PCK_ERR_BUFFULL		1
#define PCK_ERR_CRC		2





/* these are the simple callback types */
#define PCK_SMP_LOGINOK		1
#define PCK_SMP_QUEUEINFO	2
#define PCK_SMP_MSGFROMQUEUE	3


#define PCK_SMP_CLNTCAPREJ	-1
#define PCK_SMP_AUTHREJ		-2












#endif
