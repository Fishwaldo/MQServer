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
** $Id: client.c 30 2004-09-26 13:54:02Z Fish $
*/

#include <fcntl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "list.h"
#include "serversock.h"
#include "queuemanager.h"

/* this function is called direct from the packet decode library */

int MQS_Callback(void *mqplib, mqpacket *mqp) {
	mqsock *mqs = mqp->cbarg;
	nlog(LOG_DEBUG1, LOG_CORE, "Got Callback for fd %d for %ld", mqp->sock, mqs->status);

	switch (mqp->inmsg.MSGTYPE) {
		case PCK_CLNTCAP:
			nlog(LOG_DEBUG1, LOG_CORE, "Got ClntCap on fd %d", mqp->sock);
			pck_send_ack(mqplib, mqp, mqp->inmsg.MID);
			MQC_SET_STAT_AUTHSTART(mqs);
			break;
		case PCK_AUTH:
 			nlog(LOG_DEBUG1, LOG_CORE, "Got Client Auth on fd %d: %s %s", mqp->sock, mqp->inmsg.data.auth.username, mqp->inmsg.data.auth.password); 
			/* XXX Do Auth */
			qm_newauthqitm(mqs, mqp->inmsg.data.auth.username, mqp->inmsg.data.auth.password, mqp->inmsg.MID);
			break;
		case PCK_SENDTOQUEUE:
			nlog(LOG_DEBUG1, LOG_CORE, "Got Messgae from Client on fd %d for queue %s (datasize %d msgsize %d)", mqp->sock, mqp->inmsg.data.stream.queue, mqp->inmsg.data.stream.datalen, mqp->inmsg.data.stream.len);
			
			break;
		default:
			nlog(LOG_WARNING, LOG_CORE, "Got Unhandled Msgtype on fd %d", mqp->sock);
			return NS_FAILURE;
			break;
	
	}
	return NS_SUCCESS;
}


int MQS_Auth_Callback(authqitm *aqi) {
	mqsock *mqs;
	mqs = find_con_by_id(aqi->conid);
	if (mqs) {
		if (aqi->result == NS_SUCCESS) {
			MQC_SET_STAT_AUTHOK(mqs);
			pck_send_ack(mqssetup.mqplib, mqs->mqp, aqi->mid);
			qm_newclnt(mqs, aqi);
		} else { 
			pck_send_error(mqssetup.mqplib, mqs->mqp, "Invalid Username/Password");
			MQS_remove_client(mqs);
		}
	} else {
		nlog(LOG_WARNING, LOG_CORE, "Can't Find Conid %ld for Auth", aqi->conid);
	}
	return NS_SUCCESS;
}
