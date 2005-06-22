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

#ifndef LIBMQ_H
#define LIBMQ_H

#define _GNU_SOURCE 1

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <setjmp.h>
#include <assert.h>
#include "config.h"
#include "list.h"
#include "hash.h"

/* Temp disable for upcoming release until all external modules 
 * have been released with warnings fixed
 */
#if 0
#define __attribute__(x)  /* NOTHING */
#else
/* If we're not using GNU C, elide __attribute__ */
#ifndef __GNUC__
#define __attribute__(x)  /* NOTHING */
#endif
#endif


#define BUFSIZE			512

#ifndef MAXHOST
#define MAXHOST			128
#endif

#ifndef MAXPASS
#define MAXPASS			32
#endif

#ifndef MAXUSER
#define MAXUSER			15
#endif

#ifndef MAXCONNECTIONS
#define MAXCONNECTIONS		10
#endif

/* MAXPATH 
 * used to determine buffer sizes for file system operations
 */
#ifndef MAXPATH
#define MAXPATH			1024
#endif /* MAXPATH */

#define bzero(x, y)		memset(x, '\0', y);

#define NS_SUCCESS			 1
#define NS_FAILURE			-1

typedef enum LOG_LEVEL {
    MQLOG_CRITICAL=5,
    MQLOG_WARNING,
    MQLOG_NORMAL,
    MQLOG_INFO,
} LOG_LEVEL;

typedef enum DEBUG_LEVEL {
    MQDBG1=1,
    MQDBG2,
    MQDBG3,
    MQDBG4,
} DEBUG_LEVEL;

/* logging defines */
#ifdef DEBUG
#define TRACE(w,x,...) dotrace(w, x,__FILE__, __LINE__, __PRETTY_FUNCTION__,__VA_ARGS__)
#else 
#define TRACE(x,y,...);
#endif



#endif

