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

void pck_logger(char *fmt,...);

int main() {
	int connect;
	struct sockaddr_in sa;
	struct hostent *hp;

	pck_set_logger(pck_logger);	
	pck_set_server();
	pck_init();
	connect = 0;
	
	if ((hp = gethostbyname("localhost")) == NULL) {
		printf("gethostbyname failed\n");
		exit(-1);
	}
	sa.sin_family = AF_INET;
	sa.sin_port = htons(8888);
	bcopy(hp->h_addr, (char *) &sa.sin_addr, hp->h_length);
	
	
	while (1) {
		pck_process();
		sleep(2);
		if (connect == 0) {
			if (pck_make_connection(sa, NULL) == NS_FAILURE) {
				printf("connect failed\n");
				exit(-1);
			} else {
				connect = 1;
			}
		}
	}

}

                                                

void pck_logger(char *fmt,...) {
	va_list ap;
	char log_buf[BUFSIZE];
	va_start(ap, fmt);
	vsnprintf(log_buf, BUFSIZE, fmt, ap);
	va_end(ap);
	printf("MQ: %s\n", log_buf);
}
