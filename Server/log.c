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

#include "config.h"
#include "defines.h"
#include "conf.h"
#include "hash.h"
#include "log.h"
#include "dotconf.h"
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

const char* CoreLogFileName="MQServer";
char LogFileNameFormat[MAX_LOGFILENAME]="-%m-%d";

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

static char log_buf[BUFSIZE];
static char log_fmttime[TIMEBUFSIZE];

struct logs_ {
	FILE *logfile;
	char name[BUFSIZE];
	char logname[MAXPATH];
	unsigned int flush;
} logs_;

hash_t *logs;

/** @brief initialize the logging functions 
 */
int
init_logs ()
{
	SET_SEGV_LOCATION();
	logs = hash_create (-1, 0, 0);
	if (!logs) {
		printf ("ERROR: Can't initialize log subsystem. Exiting!");
		return NS_FAILURE;
	}
	printf("Logging Subsystem Started\n");
	return NS_SUCCESS;
}

/** @brief Occasionally flush log files out 
 */
void
close_logs ()
{
	hscan_t hs;
	hnode_t *hn;
	struct logs_ *logentry;

	SET_SEGV_LOCATION();
	hash_scan_begin (&hs, logs);
	while ((hn = hash_scan_next (&hs)) != NULL) {
		logentry = hnode_get (hn);
		logentry->flush = 0;
#ifdef DEBUG
		printf ("Closing Logfile %s (%s)\n", logentry->name, (char *) hnode_getkey (hn));
#endif
		if(logentry->logfile)
		{
			fflush (logentry->logfile);
			fclose (logentry->logfile);
			logentry->logfile = NULL;
		}
		hash_scan_delete (logs, hn);
		hnode_destroy (hn);
		free (logentry);
	}
}

void 
fini_logs() {
	hscan_t hs;
	hnode_t *hn;
	struct logs_ *logentry;

	SET_SEGV_LOCATION();
	hash_scan_begin (&hs, logs);
	while ((hn = hash_scan_next (&hs)) != NULL) {
		logentry = hnode_get (hn);
		logentry->flush = 0;
#ifdef DEBUG
		printf ("Closing Logfile %s (%s)\n", logentry->name, (char *) hnode_getkey (hn));
#endif
		if(logentry->logfile)
		{
			fflush (logentry->logfile);
			fclose (logentry->logfile);
			logentry->logfile = NULL;
		}
		hash_scan_delete (logs, hn);
		hnode_destroy (hn);
		free (logentry);
	}
	/* for some reason, the logs are not getting flushed correctly */
	hash_destroy(logs);
}

void make_log_filename(char* modname, char *logname)
{
	time_t t = time(NULL);
	strftime (log_fmttime, TIMEBUFSIZE, LogFileNameFormat, localtime (&t));
//	strlwr(modname);
	snprintf (logname, MAXPATH, "logs/%s%s.log", modname, log_fmttime);
}

/** @Configurable logging function
 */
void
nlog (int level, int scope, char *fmt, ...)
{
	va_list ap;
	hnode_t *hn;
	struct logs_ *logentry;
	
	if (level <= config.debug) {
		/* if scope is > 0, then log to a diff file */
		if (scope > 0) {
			if (segv_inmodule[0] != 0) {
				hn = hash_lookup (logs, segv_inmodule);
			} else {
#if 0
				nlog (LOG_ERROR, LOG_CORE, "Warning, nlog called with LOG_MOD, but segv_inmodule is blank! Logging to Core");
#endif
				hn = hash_lookup (logs, CoreLogFileName);
			}
		} else {
			hn = hash_lookup (logs, CoreLogFileName);
		}
		if (hn) {
			/* we found our log entry */
			logentry = hnode_get (hn);
			if(!logentry->logfile)
				logentry->logfile = fopen (logentry->logname, "a");
		} else {
			/* log file not found */
			if (segv_inmodule[0] == 0 && (scope > 0)) {
#ifdef DEBUG
				printf ("segv_inmodule is blank, but scope is for Modules!\n");
#endif
				/* bad, but hey ! */
				scope = 0;
			}
			logentry = malloc (sizeof (struct logs_));
			strncpy (logentry->name, scope > 0 ? segv_inmodule : CoreLogFileName, BUFSIZE);
			make_log_filename(logentry->name, logentry->logname);
			logentry->logfile = fopen (logentry->logname, "a");
			logentry->flush = 0;
			hn = hnode_create (logentry);
			hash_insert (logs, hn, logentry->name);

		}

#ifdef DEBUG
		if (!logentry->logfile) {
			printf ("LOG ERROR: %s\n", strerror (errno));
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

		fprintf (logentry->logfile, "(%s) %s %s - %s\n", log_fmttime, loglevels[level - 1], scope > 0 ? segv_inmodule : "CORE", log_buf);
		logentry->flush = 1;
#ifndef DEBUG
		if (config.foreground)
#endif
			printf ("%s %s - %s\n", loglevels[level - 1], scope > 0 ? segv_inmodule : "CORE", log_buf);
	}
}

/** rotate logs, called at midnight
 */
void
reset_logs ()
{
	hscan_t hs;
	hnode_t *hn;
	struct logs_ *logentry;

	SET_SEGV_LOCATION();
	hash_scan_begin (&hs, logs);
	while ((hn = hash_scan_next (&hs)) != NULL) {
		logentry = hnode_get (hn);
		/* If file handle is vald we must have used the log */
		if(logentry->logfile) {
			if (logentry->flush > 0) {
				fflush (logentry->logfile);
			}
			fclose (logentry->logfile);		
			logentry->logfile = NULL;
		}
		logentry->flush = 0;
#ifdef DEBUG
		printf ("Closing Logfile %s (%s)\n", logentry->name, (char *) hnode_getkey (hn));
#endif
		/* make new file name but do not open until needed to avoid 0 length files*/
		make_log_filename(logentry->name, logentry->logname);
	}
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

