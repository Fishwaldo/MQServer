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

#include "defines.h"
#include "dotconf.h"
#include "conf.h"
#include "log.h"

static void cb_Server (char *, int);

/** @brief Core Configuration Items
 * 
 * Contains Configuration Items for the Core NeoStats service
 */
static config_option options[] = {
	{"SERVER_NAME", ARG_STR, cb_Server, 0},
	{"SERVER_PORT", ARG_STR, cb_Server, 1},
	{"CONNECT_TO", ARG_STR, cb_Server, 2},
	{"CONNECT_PASS", ARG_STR, cb_Server, 3},
	{"SERVER_INFOLINE", ARG_STR, cb_Server, 4},
	{"STATSERV_NETNAME", ARG_STR, cb_Server, 5},
	{"RECONNECT_TIME", ARG_STR, cb_Server, 6},
	{"NEOSTAT_HOST", ARG_STR, cb_Server, 7},
	{"NEOSTAT_USER", ARG_STR, cb_Server, 8},
	{"WANT_PRIVMSG", ARG_STR, cb_Server, 9},
	{"SERVICES_CHAN", ARG_STR, cb_Server, 10},
	{"ONLY_OPERS", ARG_STR, cb_Server, 11},
	{"NO_LOAD", ARG_STR, cb_Server, 12},
	{"BINDTO", ARG_STR, cb_Server, 13},
	{"LOGFILENAMEFORMAT", ARG_STR, cb_Server, 14},
	{"SERVER_NUMERIC", ARG_STR, cb_Server, 15},
	{"SETSERVERTIMES", ARG_STR, cb_Server, 16},
};

/** @brief initialize the configuration parser
 *
 * Currently does nothing
 *
 * @return Nothing
 */

void
init_conf ()
{
}

/** @brief Load the Config file
 *
 * Parses the Configuration File and optionally loads the external authentication libary
 *
 * @returns Nothing
 */

int
ConfLoad ()
{
	/* Read in the Config File */
	printf ("Reading the Config File. Please wait.....\n");
	if (!config_read (CONFIG_NAME, options) == 0) {
		printf ("***************************************************\n");
		printf ("*                  Error!                         *\n");
		printf ("*                                                 *\n");
		printf ("* Config File not found, or Unable to Open        *\n");
		printf ("* Please check its Location, and try again        *\n");
		printf ("*                                                 *\n");
		printf ("*             NeoStats NOT Started                *\n");
		printf ("***************************************************\n");
		return NS_FAILURE;
	}
	printf ("Sucessfully Loaded Config File, Now Booting NeoStats\n");

	return NS_SUCCESS;
}


/** @brief Process config file items
 *
 * Processes the config file and sets up the variables. No Error Checking is performed :(
 *
 * @param arg the variable value as a string
 * @param configtype the index of the variable being called now
 * @returns Nothing
 */

void
cb_Server (char *arg, int configtype)
{
	if (configtype == 0) {
		/* Server name */
		strncpy (me.name, arg, sizeof (me.name));
	} else if (configtype == 1) {
		/* Server Port */
		me.port = atoi (arg);
	} else if (configtype == 2) {
	} else if (configtype == 3) {
		/* Connect Pass */
		strncpy (me.pass, arg, sizeof (me.pass));
	} else if (configtype == 4) {
	} else if (configtype == 5) {
	} else if (configtype == 6) {
		/* Reconnect time */
		me.r_time = atoi (arg);
	} else if (configtype == 7) {
		/* NeoStat Host */
		strncpy (me.host, arg, MAXHOST);
	} else if (configtype == 8) {
		/* NeoStat User */
		strncpy (me.user, arg, MAXUSER);
	} else if (configtype == 9) {
	} else if (configtype == 10) {
	} else if (configtype == 11) {
	} else if (configtype == 12) {
		me.die = 1;
	} else if (configtype == 13) {
		strncpy (me.local, arg, sizeof (me.local));
	} else if (configtype == 14) {
		strncpy(LogFileNameFormat,arg,MAX_LOGFILENAME);
	} else if (configtype == 15) {
	} else if (configtype == 16) {
	}

}

/** @brief Rehash Function
 *
 * Called when we recieve a rehash signal. Does nothing atm
 *
 * @returns Nothing
 */

void
rehash ()
{
	/* nothing, yet */
}
