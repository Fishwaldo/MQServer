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

#ifndef DEFINES_H
#define DEFINES_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#define __USE_GNU
#include <string.h>
#undef __USE_GNU
#include <stdarg.h>
#include <stdlib.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <setjmp.h>
#include <assert.h>
#include "config.h"

#ifdef HAVE_DB_H
/*#define USE_BERKELEY*/
#endif

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

#include "list.h"
#include "hash.h"

#ifdef MQSERVER_REVISION
#define MQSERVER_VERSION MQSERVER_PACKAGE_VERSION " (" MQSERVER_REVISION ")"
#else
#define MQSERVER_VERSION MQSERVER_PACKAGE_VERSION
#endif

#define CONFIG_NAME		"mqserver.cfg"
#define PID_FILENAME    	"mqserver.pid"

#define BUFSIZE			512
#ifndef MAXHOST
#define MAXHOST			128
#endif
#ifndef MAXPASS
#define MAXPASS			32
#endif
#ifndef MAXNICK
#define MAXNICK			32
#endif
#ifndef MAXUSER
#define MAXUSER			15
#endif
#ifndef MAXREALNAME
#define MAXREALNAME		50
#endif

#ifndef DNS_QUEUE_SIZE
#define DNS_QUEUE_SIZE		20
#endif


/* MAXPATH 
 * used to determine buffer sizes for file system operations
 */
#ifndef MAXPATH
#define MAXPATH			1024
#endif /* MAXPATH */

/* TIMEBUFSIZE
 * used to determine buffer sizes for time formatting buffers
 */
#define TIMEBUFSIZE		80

/* STR_TIME_T_SIZE
 * size of a time_t converted to a string. 
 */
#define STR_TIME_T_SIZE	24

/* Buffer size for version string */
#define VERSIONSIZE		32

#define bzero(x, y)		memset(x, '\0', y);

/* Early creation of unified return values and error system */
/* These are program exit codes usually defined in stdlib.h but 
   if not found will be defined here */
#ifndef EXIT_FAILURE 
#define EXIT_FAILURE 1
#endif /* EXIT_FAILURE */
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif /* EXIT_SUCCESS */

#define NS_SUCCESS			 1
#define NS_FAILURE			-1

/* Specific errors beyond SUCCESS/FAILURE so that functions can handle errors 
 * Treat as unsigned with top bit set to give us a clear distinction from 
 * other values and use a typedef ENUM so that we can indicate return type */
typedef enum NS_ERR {
	NS_ERR_NICK_IN_USE		= 0x8000001,
	NS_ERR_OUT_OF_MEMORY	= 0x8000002,
	NS_ERR_VERSION			= 0x8000003,
}NS_ERR ;


/* do_exit call exit type definitions */
typedef enum {
	NS_EXIT_NORMAL=0,
	NS_EXIT_RELOAD,
	NS_EXIT_RECONNECT,
	NS_EXIT_ERROR,
	NS_EXIT_SEGFAULT,
}NS_EXIT_TYPE;

#define SEGV_LOCATION_BUFSIZE	255
#define SET_SEGV_LOCATION() snprintf(segv_location,SEGV_LOCATION_BUFSIZE,"%s %d %s", __FILE__, __LINE__, __PRETTY_FUNCTION__); 
#define SET_SEGV_LOCATION_EXTRA(debug_text) snprintf(segv_location,SEGV_LOCATION_BUFSIZE,"%s %d %s %s", __FILE__, __LINE__, __PRETTY_FUNCTION__,(debug_text)); 
#define CLEAR_SEGV_LOCATION() segv_location[0]='\0';

#define SEGV_INMODULE_BUFSIZE	255
#define SET_SEGV_INMODULE(module_name) strncpy(segv_inmodule,(module_name),SEGV_INMODULE_BUFSIZE);
#define CLEAR_SEGV_INMODULE() segv_inmodule[0]='\0';

/* macros to provide a couple missing string functions for code legibility 
 * and to ensure we perform these operations in a standard and optimal manner
 */
/* set a string to NULL */
#define strsetnull(str) (str)[0] = 0
/* test a string for NULL */
#define strisnull(str)  ((str) && (str)[0] == 0)

#define ARRAY_COUNT (a) ((sizeof ((a)) / sizeof ((a)[0]))

extern char segv_location[SEGV_LOCATION_BUFSIZE];
extern char segv_inmodule[SEGV_INMODULE_BUFSIZE];
extern jmp_buf sigvbuf;


/* version info */
extern const char version_date[], version_time[];

/** @brief me structure
 *  structure containing information about the neostats core
 */
struct me {
	char name[MAXHOST];
	int port;
	int r_time;
	char pass[MAXPASS];
	char local[MAXHOST];
	char user[MAXUSER];			/* bot user */
	char host[MAXHOST];			/* bot host */
	char realname[MAXREALNAME];	/* bot real name */
	time_t t_start;
	unsigned int die:1;
	unsigned int debug_mode:1;
	time_t now;
	char strnow[STR_TIME_T_SIZE];
	char version[VERSIONSIZE];
	char versionfull[VERSIONSIZE];
} me;

/* conf.c */
int ConfLoad (void);
void rehash (void);
int ConfLoadModules (void);

/* main.c */
void do_exit (NS_EXIT_TYPE exitcode, char* quitmsg) __attribute__((noreturn));
void fatal_error(char* file, int line, char* func, char* error_text) __attribute__((noreturn));;
#define FATAL_ERROR(error_text) fatal_error(__FILE__, __LINE__, __PRETTY_FUNCTION__,(error_text)); 

/* misc.c */
void strip (char * line);
void *smalloc (long size);
char *sstrdup (const char * s);
char *strlwr (char * s);
void AddStringToList (char ***List, char S[], int *C);
char *sctime (time_t t);
char *sftime (time_t t);



#endif

