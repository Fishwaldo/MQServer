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
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
                     
#include "defines.h"
#include "adns.h"
#include "conf.h"
#include "log.h"
#include "dotconf.h"
#include "client.h"
#include "dns.h"
#include "packet.h"
#include "getchar.h"

int servsock;

static int listen_on_port(int port);

static void setup_dns_socks();

static int buffer_add(mqclient *mqc, void *data, size_t datlen);

int main() {
	u_char outbuf[BUFSIZE];
	u_char *buf;
	u_long mid = 2131;
	u_short mtyp = 2;
	u_short ver = 1;
	u_long len;
	u_long flag = 0;
	u_long crc = 12121112;
	char mydata[] = "This is my data string\0";
	int fd;
	
	len = strlen(mydata);
	fd = ConnectTo("10.1.1.12", 1234);
	printf("%d\n", fd);
	if (fd < 0) {
		printf("failed connect\n");
		exit(-1);
	}
	bzero(outbuf, BUFSIZE);
	buf = outbuf;
	PUTLONG(mid, buf);
	PUTSHORT(mtyp, buf);
	PUTSHORT(ver, buf);
	PUTLONG(len, buf);
	PUTLONG(flag, buf);
	PUTLONG(crc, buf);
	memcpy(buf, mydata, strlen(mydata));
	write(fd, outbuf, BUFSIZE);
	fsync(fd);
	sleep(1);
	write(fd, outbuf, BUFSIZE);
	fsync(fd);
	sleep(1);
	write(fd, outbuf, BUFSIZE);
	fsync(fd);
	sleep(1);
	write(fd, outbuf, BUFSIZE);
	fsync(fd);
	sleep(1);
	write(fd, outbuf, BUFSIZE);
	fsync(fd);
	sleep(1);
	printf("%d\n", write(fd, outbuf, BUFSIZE));
	fsync(fd);

			

}

/** @brief Connect to a server
 *
 *  also setups the SQL listen socket if defined 
 *
 * @param host to connect to
 * @param port on remote host to connect to
 * 
 * @return socket connected to on success
 *         NS_FAILURE on failure 
 */
int
ConnectTo (char *host, int port)
{
	int ret;
	struct hostent *hp;
	struct sockaddr_in sa;
	int s;

	/* bind to a local ip */
	if ((hp = gethostbyname (host)) == NULL) {
		return NS_FAILURE;
	}

	if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
		free(hp);
		return NS_FAILURE;
	}
	bzero (&sa, sizeof (sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons (port);
	bcopy (hp->h_addr, (char *) &sa.sin_addr, hp->h_length);

	ret=connect (s, (struct sockaddr *) &sa, sizeof (sa));
	if (ret< 0) {
		close (s);
		return NS_FAILURE;
	}
	return s;
}

