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
#include "list.h"


#ifndef INPACK_MAX
#define INPACK_MAX 100
#endif

#ifndef OUTPACK_MAX
#define OUTPACK_MAX 100
#endif

#define PCK_MIN_PACK_SIZE 84


list_t *connections;


#define MQP_CLIENTAUTHED 0x01
#define MQP_CLIENTSETUP 0x02

#define MQP_IS_AUTHED(x) (x->flags & MQP_CLIENTAUTHED)
#define MQP_SET_AUTHED(x) (x->flags |= MQP_CLIENTAUTHED)

#define MQP_IS_SETUP(x) ((x->flags & MQP_CLIENTSETUP) && (x->flags & MQP_CLIENTAUTHED))
#define MQP_SET_SETUP(x) (x->flags |= MQP_CLIENTSETUP)



/* client protocol struct */
typedef struct mqprotocol {
	list_t *inpack;
	unsigned long nxtinmid;
	list_t *outpack;
	unsigned long nxtoutmid;
	int wtforinpack;
	int servorclnt;
	int sock;
	int pollopts;
	int flags;
	void *cbarg;
	void *buffer;
	size_t bufferlen;
	size_t offset;
	void *outbuffer;
	size_t outbufferlen;
	size_t outoffset;
} mqprotocol;	

typedef void (logfunc)(char *fmt, ...)  __attribute__((format(printf,1,2))); /* 3=format 4=params */
typedef int (connectauthfunc)(int fd, struct sockaddr_in);

struct mqpconfig {
	logfunc *logger;
	connectauthfunc *connectauth;
	int server;
	int port;
	int listenfd;
} mqpconfig; 	 

/* server or client or repl flags mqprotocol->servorclnt */
#define PCK_IS_CLIENT	0x1
#define PCK_IS_SERVER	0x2
#define PCK_IS_REPL	0x4

#define MQP_IS_CLIENT(x) (x->servorclnt & PCK_IS_CLIENT)
#define MQP_IS_SERVER(x) (x->servorclnt & PCK_IS_SERVER)
#define MQP_IS_REPL(x) (x->servorclnt & PCK_IS_REPL)

/* individual packets struct */
typedef struct mqpacket {
	unsigned long MID;
	int MSGTYPE;
	int VERSION;
	unsigned long flags;
	void *data;
	unsigned long dataoffset;
	xds_t *xds;
} mqpacket;


/* flags to indicate the connection status */
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

/* packet flags, not message flags */
#define PCK_FLG_REQUIREACK		0x01
#define PCK_FLG_REQUIREACPPROCESS 	0x02


/* encoding engine structs */
#define NUMENGINES 9

#define ENG_TYPE_XDR 1
#define ENG_TYPE_XML 2

typedef struct myengines {
	xds_engine_t ptr;
	const char myname[BUFSIZE];
}myengines;

extern myengines enc_xdr_engines[NUMENGINES];




extern int encode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args);
extern int decode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args);




void pck_init();
void pck_set_logger(logfunc *logger);
mqprotocol *pck_new_conn(void *cbarg, int type);
int pck_parse_packet(mqprotocol *mqp, u_char *buffer, unsigned long buflen);


void pck_destroy_mqpacket(mqpacket *mqpck, mqprotocol *mqp);
mqpacket *pck_create_mqpacket(int type, xds_mode_t direction);
lnode_t *pck_find_mid_node(unsigned long MID, list_t *queue);
mqpacket *pck_new_packet (int msgtype, unsigned long flags);
unsigned long pck_commit_data (mqprotocol * mqp, mqpacket * mqpck);
void pck_set_server();
int pck_process();
int pck_make_connection(struct sockaddr_in sa, void *cbarg);


/* these are error defines */
#define PCK_ERR_BUFFULL		1
#define PCK_ERR_CRC		2
#endif
