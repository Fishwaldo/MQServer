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

#ifndef _log_h_
#define _log_h_

/*
 * log.h
 * dynamic configuration runtime libary
 */

/* define the log levels */

/* critcal crash type notices */
#define LOG_CRITICAL   	1
/* something is majorly wrong */
#define LOG_ERROR	2
/* Hey, you should know about this type messages */
#define LOG_WARNING	3
/* did you know messages */
#define LOG_NOTICE	4
/* our normal logging level? */
#define LOG_NORMAL	5
/* lots of info about what we are doing */
#define LOG_INFO	6
/* debug notices about important functions that are going on */
#define LOG_DEBUG1	7
/* more debug notices that are usefull */
#define LOG_DEBUG2	8
/* even more stuff, that would be useless to most normal people */
#define LOG_DEBUG3	9
/* are you insane? */
#define LOG_DEBUG4	10

/* Scope of Logging Defines: */

#define LOG_CORE	0
#define LOG_AUTHQ	1
#define LOG_MESSQ	2
#define LOG_PROT	3

/* buffer size for new customisable filenames 
   20 should be more than sufficient */
#define MAX_LOGFILENAME	20



/* XXX make this go away */
#define fini_logs close_logs


/* this is for the neostats assert replacement. */
/* Version 2.4 and later of GCC define a magical variable _PRETTY_FUNCTION__'
   which contains the name of the function currently being defined.
   This is broken in G++ before version 2.6.
   C9x has a similar variable called __func__, but prefer the GCC one since
   it demangles C++ function names.  */

#define __NASSERT_FUNCTION    __PRETTY_FUNCTION__
/* Not all compilers provide __STRING so define it here if it is unknown */
#ifndef __STRING
#define __STRING(x) #x
#endif /* __STRING */ 


#ifndef __ASSERT_VOID_CAST
#define __ASSERT_VOID_CAST (void)
#endif
extern void nassert_fail (const char *expr, const char *file, const int line, const char *infunk);

#ifndef NDEBUG
#define nassert(expr) \
  (__ASSERT_VOID_CAST ((expr) ? 0 :                                           \
	(nassert_fail(__STRING(expr), __FILE__, __LINE__, __NASSERT_FUNCTION), 0)))
#else
#define nassert(expr) (__ASSERT_VOID_CAST (0))
#endif

extern void nlog (int level, int scope, char *fmt, ...) __attribute__((format(printf,3,4))); /* 2=format 3=params */
void close_logs ();
int init_logs ();
void reset_logs ();
void fini_logs();
/* Configurable log filename format string */
extern char LogFileNameFormat[MAX_LOGFILENAME];

#if SQLSRV
void sqlsrvlog(char *logline);
#endif

#endif
