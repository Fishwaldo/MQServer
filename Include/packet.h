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
#define PCK_ACK_FMT		"int32"
#define PCK_ERROR_FMT		"string"
#define PCK_SRVCAP_FMT		"int32 int32 string"
#define PCK_CLNTCAP_FMT		"int32 int32 string"
#define PCK_AUTH_FMT		"string string"

/* packet flags, not message flags */
#define PCK_FLG_REQUIREACK		0x01
#define PCK_FLG_REQUIREACPPROCESS 	0x02


struct mq_data_stream {
	void *data;
	size_t len;
} mq_data_stream;

struct mq_data_srvcap {
	int srvcap1;
	int srvcap2;
	char *capstr;
} mq_data_srvcap;

struct mq_data_auth {
	char *username;
	char *password;
	char host[BUFSIZE];
	long flags;
} mq_data_auth;

struct message {
	int MID;
	int MSGTYPE;
	int VERSION;
	int flags;
	union {
		struct mq_data_stream stream;
		struct mq_data_srvcap srvcap;
		struct mq_data_auth auth;
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
unsigned long send_ack(mqp *mqplib, mqpacket *mqp, int MID);




/* this is the standalone un-threadsafe interface */
int init_socket();
int debug_socket(int i);
int enable_server(int port);
int pck_process ();
int pck_make_connection (char *hostname, char *, char *, long , void *cbarg);



/* these are error defines */
#define PCK_ERR_BUFFULL		1
#define PCK_ERR_CRC		2


















#endif
