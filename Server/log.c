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

#include <pthread.h>

#include "config.h"
#include "defines.h"
#include "conf.h"
#include "hash.h"
#include "log.h"
#include "mythread.h"
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

const char* CoreLogFileName="MQServer";
char LogFileNameFormat[MAX_LOGFILENAME]="-%m-%d";


mylocks logmutex;

const char *loglevels[10] = {
	"CRITICAL",
	"ERROR",
	"WARNING",
	"NOTICE",
	"NORMAL",
	"INFO",
	"DEBUG1",
	"DEBUG2",
	"DEBUG3",
	"INSANE"
};

const char *logscope[] = {
	"CORE",
	"AUTH",
	"QUEUE",
	"PROTOCOL"
};

static char log_buf[BUFSIZE];
static char log_fmttime[TIMEBUFSIZE];

struct mqlog {
	FILE *logfile;
	char filename[MAXPATH];
} mqlog;
	

/** @brief initialize the logging functions 
 */
int
init_logs ()
{
	SET_SEGV_LOCATION();

	MYLOCK_INIT(logmutex);
	MYLOCK(&logmutex);
	snprintf (mqlog.filename, MAXPATH, "logs/%s", CoreLogFileName);

	printf("Logging Subsystem Started\n");
	MYUNLOCK(&logmutex);
	return NS_SUCCESS;
}


/** @brief Occasionally flush log files out 
 */
void
close_logs ()
{
	SET_SEGV_LOCATION();

	MYLOCK(&logmutex);
#ifdef DEBUG
		printf ("Closing Logfile %s\n", mqlog.filename);
#endif
		if(mqlog.logfile)
		{
			fflush (mqlog.logfile);
			fclose (mqlog.logfile);
		}
	MYUNLOCK(&logmutex);
}

/** @Configurable logging function
 */
void
nlog (int level, int scope, char *fmt, ...)
{
	va_list ap;
	
	MYLOCK(&logmutex);
	
	if (level <= config.debug) {
		if(!mqlog.logfile)
			mqlog.logfile = fopen (mqlog.filename, "a");
#ifdef DEBUG
		if (!mqlog.logfile) {
			printf ("LOG ERROR: %s\n", strerror (errno));
			MYUNLOCK(&logmutex);
			do_exit (NS_EXIT_NORMAL, NULL);
		}
#endif
		/* we update me.now here, becase some functions might be busy and not call the loop a lot */
		me.now = time(NULL);
		snprintf (me.strnow, STR_TIME_T_SIZE, "%ld", me.now);
		strftime (log_fmttime, TIMEBUFSIZE, "%d/%m/%Y[%H:%M:%S]", localtime (&me.now));
		va_start (ap, fmt);
		vsnprintf (log_buf, BUFSIZE, fmt, ap);
		va_end (ap);

		fprintf (mqlog.logfile, "(%s) %s %s - %s\n", log_fmttime, loglevels[level - 1], logscope[scope], log_buf);
#ifndef DEBUG
		if (config.foreground)
#endif
			printf ("%s %s - %s\n", loglevels[level - 1], logscope[scope], log_buf);
	}
	MYUNLOCK(&logmutex);
}

/** rotate logs, called at midnight
 */
void
reset_logs ()
{

	SET_SEGV_LOCATION();
	MYLOCK(&logmutex);
	/* XXX TODO */
	MYUNLOCK(&logmutex);
}

/* this is for printing out details during an assertion failure */
void
nassert_fail (const char *expr, const char *file, const int line, const char *infunk)
{
#ifdef HAVE_BACKTRACE
	void *array[50];
	size_t size;
	char **strings;
	size_t i;
/* thanks to gnulibc libary for letting me find this usefull function */
	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);
#endif

	nlog (LOG_CRITICAL, LOG_CORE, "Assertion Failure!!!!!!!!!!!");
	nlog (LOG_CRITICAL, LOG_CORE, "Function: %s (%s:%d)", infunk, file, line);
	nlog (LOG_CRITICAL, LOG_CORE, "Expression: %s", expr);
#ifdef HAVE_BACKTRACE
	for (i = 1; i < size; i++) {
		nlog (LOG_CRITICAL, LOG_CORE, "BackTrace(%d): %s", i - 1, strings[i]);
	}
#endif
	nlog (LOG_CRITICAL, LOG_CORE, "Shutting Down!");
	exit (EXIT_FAILURE);
}

