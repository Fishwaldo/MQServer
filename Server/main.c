/* MQServer 
** Copyright (c) 2004 Justin Hammond
** http://www.dynam.ac/
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
** MQServer CVS Identification
** $Id$
*/

#include "config.h"
#include <setjmp.h>
#include <stdio.h>
#include <pthread.h>
#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif
#include "defines.h"
#include "signal.h"
#include "log.h"
#include "sock.h"
#include "dns.h"
#include "packet.h"
#include "serversock.h"
#include "mythread.h"
#include "queuemanager.h"
#include "conf.h"
#include "client.h"
#include "messagequeue.h"

/*! Date when we were compiled */
const char version_date[] = __DATE__;
/*! Time we were compiled */
const char version_time[] = __TIME__;

char segv_inmodule[SEGV_INMODULE_BUFSIZE];
char segv_location[SEGV_LOCATION_BUFSIZE];

void start (void);
static void setup_signals (void);
static int get_options (int argc, char **argv);

void pck_logger(char *fmt,...);

/*! have we forked */
int forked = 0;
static int attempts = 0;
jmp_buf sigvbuf;

/** @brief Main Entry point into program
 *
 * Sets up defaults, and parses the config file.
 * Also initializes different parts of NeoStats
 * 
 * @return Exits the program!
 *
 * @todo Close STDIN etc correctly
 */
int
main (int argc, char *argv[])
{
	FILE *fp;

	/* initialise version */
	strncpy(me.version, MQSERVER_VERSION, VERSIONSIZE);
	strncpy(me.versionfull, MQSERVER_VERSION, VERSIONSIZE);

	/* get our commandline options */
	if(get_options (argc, argv)!=NS_SUCCESS)
		return EXIT_FAILURE;

#ifndef DEBUG
	/* Change to the working Directory */
	if (chdir (NEO_PREFIX) < 0) {
		printf ("MQServer Could not change to %s\n", NEO_PREFIX);
		printf ("Did you 'make install' after compiling?\n");
		printf ("Error Was: %s\n", strerror (errno));
		return EXIT_FAILURE;
	}
#endif
	/* start up the thread engine */
	init_threadengine();


	/* before we do anything, make sure logging is setup */
	if(init_logs () != NS_SUCCESS)
		return EXIT_FAILURE;

	/* our crash trace variables */
	SET_SEGV_LOCATION();
	CLEAR_SEGV_INMODULE();

	/* keep quiet if we are told to :) */
	if (!config.quiet) {
		printf ("MQServer %s Loading...\n", me.versionfull);
		printf ("-----------------------------------------------\n");
		printf ("Copyright 2004\n");
		printf ("Justin Hammond (justin@dynam.ac)\n");
		printf ("-----------------------------------------------\n\n");
	}
	/* set some defaults before we parse the config file */
	me.t_start = time(NULL);
	me.now = time(NULL);
	snprintf (me.strnow, STR_TIME_T_SIZE, "%ld", (long)me.now);
	me.die = 0;
	me.local[0] = '\0';
#ifndef DEBUG
	me.debug_mode = 0;
#else
	me.debug_mode = 1;
#endif
	me.r_time = 10;

	/* XXX */
	me.authqthreshold = 5;
	me.authqmaxthreads = 5;	
	me.authqtimeout = 10;
	me.messqthreshold = 500;
	me.messqmaxthreads = 1;	
	me.messqtimeout = 10000;


	/* setup the mqclients list */
	mq_clients = list_create(-1);
	MYLOCK_INIT(mq_clientslock);

	/* setup the mq_queues list */
        mq_queues = hash_create(-1, 0, 0);
        MYLOCK_INIT(mq_queuelock);


	/* prepare to catch errors */
	setup_signals ();


	
	/* load the config files */
	if(ConfLoad () != NS_SUCCESS)
		return EXIT_FAILURE;

	if (me.die) {
		printf ("\n-----> ERROR: Read the README file then edit %s! <-----\n\n",CONFIG_NAME);
		nlog (LOG_CRITICAL, LOG_CORE, "Read the README file and edit your %s",CONFIG_NAME);
		/* we are exiting the parent, not the program, so just return */
		return EXIT_FAILURE;
	}

	/* initialize the rest of the subsystems */
	if(init_dns () != NS_SUCCESS)
		return EXIT_FAILURE;

	/* init the queue manager */
	if (queueman_init() != NS_SUCCESS)
		return EXIT_FAILURE;
		

#if 0
	/* if we are compiled with debug, or forground switch was specified, DONT FORK */
	if (!config.foreground) {
		/* fix the double log message problem by closing logs prior to fork() */ 
		close_logs(); 
		forked = fork ();
		/* Error check fork() */ 
		if (forked<0) { 
			perror("fork"); 
			return EXIT_FAILURE; /* fork error */ 
		} 
#endif
		/* we are the parent */ 
		if (forked>0) { 
			/* write out our PID */
			fp = fopen (PID_FILENAME, "w");
			fprintf (fp, "%i", forked);
			fclose (fp);
			if (!config.quiet) {
				printf ("\n");
				printf ("MQServer %s Successfully Launched into Background\n", me.versionfull);
				printf ("PID: %i - Wrote to %s\n", forked, PID_FILENAME);
			}
			return EXIT_SUCCESS; /* parent exits */ 
		}
#if 0
		/* child (daemon) continues */ 
		/* reopen logs for child */ 
		if(init_logs () != NS_SUCCESS)
			return EXIT_FAILURE;
		/* detach from parent process */
		if (setpgid (0, 0) < 0) {
			nlog (LOG_WARNING, LOG_CORE, "setpgid() failed");
		}
	}
#endif
	nlog (LOG_NOTICE, LOG_CORE, "MQServer started (Version %s).", me.versionfull);
	/* we are ready to start now Duh! */

	/* XXX init server socket thread */
	MQS_sock_start();


	/* We should never reach here but the compiler does not realise and may
	   complain about not all paths control returning values without the return 
	   Since it should never happen, treat as an error condition! */
	return EXIT_FAILURE;
}

/** @brief Process Commandline Options
 *
 * Processes commandline options
 *
 * returns 0 on success, -1 on error
*/
static int
get_options (int argc, char **argv)
{
	int c;
	int dbg;

	SET_SEGV_LOCATION();
	/* set some defaults first */
#ifdef DEBUG
	config.debug = 10;
	config.foreground = 1;
#else
	config.debug = 5;
	config.foreground = 0;
#endif

	while ((c = getopt (argc, argv, "hvrd:nqf")) != -1) {
		switch (c) {
		case 'h':
			printf ("MQServer: Usage: \"neostats [options]\"\n");
			printf ("         -h (Show this screen)\n");
			printf ("	  -v (Show version number)\n");
			printf ("	  -d 1-10 (Enable debugging output 1= lowest, 10 = highest)\n");
			printf ("	  -q (Quiet start - for cron scripts)\n");
			printf ("         -f (Do not fork into background\n");
			return NS_FAILURE;
		case 'v':
			printf ("MQServer Version %s\n", me.versionfull);
			printf ("Compiled: %s at %s\n", version_date, version_time);
			printf ("\nMQServer: http://www.neostats.net\n");
			return NS_FAILURE;
		case 'q':
			config.quiet = 1;
			break;
		case 'd':
			dbg = atoi (optarg);
			if ((dbg > 10) || (dbg < 1)) {
				printf ("Invalid Debug Level %d\n", dbg);
				return NS_FAILURE;
			}
			config.debug = dbg;
			break;
		case 'f':
			config.foreground = 1;
			break;
		default:
			printf ("Unknown command line switch %c\n", optopt);
		}
	}
	return NS_SUCCESS;
}



/** @brief Sigterm Signal handler
 *
 * Called by the signal handler if we get a SIGTERM
 * This shutsdown MQServer and exits
 * 
 * @return Exits the program!
 *
 * @todo Do a nice shutdown, no thtis crap :)
 */
char msg_sigterm[]="SIGTERM received, shutting down server.";

RETSIGTYPE
serv_die ()
{
	me.die = 1;
#ifdef VALGRIND
	exit(NS_SUCCESS);
#else /* VALGRIND */
	nlog (LOG_CRITICAL, LOG_CORE, msg_sigterm);

	do_exit (NS_EXIT_NORMAL, msg_sigterm);
#endif /* VALGRIND */
}

/** @brief Sighup Signal handler
 *
 * Called by the signal handler if we get a SIGHUP
 * and rehashes the config file.
 * 
 * @return Nothing
 *
 * @todo Implement a Rehash function. What can we actually rehash?
 */
RETSIGTYPE
conf_rehash ()
{
/* nothing yet */
}


/** @brief Sigsegv  Signal handler
 *
 * This function is called when we get a SEGV
 * and will send some debug into to the logs and to IRC
 * to help us track down where the problem occured.
 * if the platform we are using supports backtrace
 * also print out the backtrace.
 * if the segv happened inside a module, try to unload the module
 * and continue on our merry way :)
 * 
 * @return Nothing
 *
 */
#ifndef HAVE_BACKTRACE
static char backtrace_unavailable[]="Backtrace not available on this platform";
#endif
void do_backtrace(void)
{
#ifdef HAVE_BACKTRACE
	void *array[50];
	size_t size;
	char **strings;
	int i;

	nlog (LOG_CRITICAL, LOG_CORE, "Backtrace:");
	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);
	for (i = 1; i < size; i++) {
		nlog (LOG_CRITICAL, LOG_CORE, "BackTrace(%d): %s", i - 1, strings[i]);
	}
	free (strings);
#else
	nlog (LOG_CRITICAL, LOG_CORE, backtrace_unavailable);
#endif
}

RETSIGTYPE
serv_segv ()
{
	static int inbacktrace = 0;
	/** if the segv happened while we were inside a module, unload and try to restore 
	 *  the stack to where we were before we jumped into the module
	 *  and continue on
	 */
	if (inbacktrace == 1) {
		return;
	} else {
		inbacktrace = 1;
	}
	if (segv_inmodule[0] != 0) {
		nlog (LOG_CRITICAL, LOG_CORE, "------------------------SEGFAULT REPORT-------------------------");
		nlog (LOG_CRITICAL, LOG_CORE, "Please view the README for how to submit a bug report");
		nlog (LOG_CRITICAL, LOG_CORE, "and include this segfault report in your submission.");
		nlog (LOG_CRITICAL, LOG_CORE, "Module:   %s", segv_inmodule);
		nlog (LOG_CRITICAL, LOG_CORE, "Location: %s", segv_location);
		nlog (LOG_CRITICAL, LOG_CORE, "Unloading Module and restoring stacks. Backtrace:");
		do_backtrace();
		nlog (LOG_CRITICAL, LOG_CORE, "-------------------------END OF REPORT--------------------------");
		/* flush the logs out */
		close_logs(); 
		longjmp (sigvbuf, -1);
		inbacktrace = 0;
		return;
	}
	/** The segv happened in our core, damn it */
	/* Thanks to Stskeeps and Unreal for this stuff :) */
	/* Broadcast it out! */
	nlog (LOG_CRITICAL, LOG_CORE, "------------------------SEGFAULT REPORT-------------------------");
	nlog (LOG_CRITICAL, LOG_CORE, "Please view the README for how to submit a bug report");
	nlog (LOG_CRITICAL, LOG_CORE, "and include this segfault report in your submission.");
	nlog (LOG_CRITICAL, LOG_CORE, "Location: %s", segv_location);
	do_backtrace();
	nlog (LOG_CRITICAL, LOG_CORE, "-------------------------END OF REPORT--------------------------");
	me.die = 1;
	close_logs();
	/* clean up */
	do_exit (NS_EXIT_SEGFAULT, NULL);
}

/** @brief Sets up the signal handlers
 *
 * Sets up the signal handlers for SIGHUP (rehash)
 * SIGTERM (die) and SIGSEGV (segv fault)
 * and ignore the others (Such as SIGPIPE)
 * 
 * @return Nothing
 *
 */
static void
setup_signals (void)
{
	struct sigaction act;
	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;

	SET_SEGV_LOCATION();
	/* SIGPIPE/SIGALRM */
	(void) sigemptyset (&act.sa_mask);
	(void) sigaddset (&act.sa_mask, SIGPIPE);
	(void) sigaddset (&act.sa_mask, SIGALRM);
	(void) sigaction (SIGPIPE, &act, NULL);
	(void) sigaction (SIGALRM, &act, NULL);

	/* SIGHUP */
	act.sa_handler = conf_rehash;
	(void) sigemptyset (&act.sa_mask);
	(void) sigaddset (&act.sa_mask, SIGHUP);
	(void) sigaction (SIGHUP, &act, NULL);

	/* SIGTERM/SIGINT */
	act.sa_handler = serv_die;
	(void) sigaddset (&act.sa_mask, SIGTERM);
	(void) sigaction (SIGTERM, &act, NULL);
	(void) sigaddset (&act.sa_mask, SIGINT);
	(void) sigaction (SIGINT, &act, NULL);

    /* SIGSEGV */
	act.sa_handler = serv_segv;
	(void) sigaddset (&act.sa_mask, SIGSEGV);
	(void) sigaction (SIGSEGV, &act, NULL);

	(void) signal (SIGHUP, conf_rehash);
	(void) signal (SIGTERM, serv_die);
	(void) signal (SIGSEGV, serv_segv);
	(void) signal (SIGINT, serv_die);
}

/** @brief before exiting call this function. It flushes log files and tidy's up.
 *
 *  Cleans up before exiting 
 *  @parm segv 1 = we are exiting because of a segv fault, 0, we are not.
 *  if 1, we don't prompt to save data
 */
void
do_exit (NS_EXIT_TYPE exitcode, char* quitmsg)
{
	/* Initialise exit code to OK */
	int return_code=EXIT_SUCCESS;
	SET_SEGV_LOCATION();
	switch (exitcode) {
	case NS_EXIT_NORMAL:
		nlog (LOG_CRITICAL, LOG_CORE, "Normal shut down subsystems");
		break;
	case NS_EXIT_RELOAD:
		nlog (LOG_CRITICAL, LOG_CORE, "Reloading MQServer");
		break;
	case NS_EXIT_RECONNECT:
		nlog (LOG_CRITICAL, LOG_CORE, "Restarting MQServer subsystems");
		break;
	case NS_EXIT_ERROR:
		nlog (LOG_CRITICAL, LOG_CORE, "Exiting due to error");
		return_code=EXIT_FAILURE;	/* exit code to error */
		break;		
	case NS_EXIT_SEGFAULT:
		nlog (LOG_CRITICAL, LOG_CORE, "Shutting down subsystems without saving data due to core");
		return_code=EXIT_FAILURE;	/* exit code to error */
		break;
	}

	if (exitcode != NS_EXIT_SEGFAULT) {
		if (exitcode == NS_EXIT_RECONNECT) {
			if(me.r_time>0) {
				nlog (LOG_NOTICE, LOG_CORE, "Reconnecting to the server in %d seconds (Attempt %i)", me.r_time, attempts);
				sleep (me.r_time);
			}
			else {
				nlog (LOG_NOTICE, LOG_CORE, "Reconnect time is zero, shutting down");
			}
		}

	}

	fini_logs();
	me.die = 1;
#if 0
	if ((exitcode == NS_EXIT_RECONNECT && me.r_time > 0) || exitcode == NS_EXIT_RELOAD) {
		execve ("./mqserver", NULL, NULL);
		return_code=EXIT_FAILURE;	/* exit code to error */
	}
#endif
	remove (PID_FILENAME);
	exit (return_code);
}

void fatal_error(char* file, int line, char* func, char* error_text)
{
	SET_SEGV_LOCATION();
	nlog (LOG_CRITICAL, LOG_CORE, "Fatal Error: %s %d %s %s", file, line, func, error_text);
	do_exit (NS_EXIT_ERROR, "Fatal Error - check log file");
}

