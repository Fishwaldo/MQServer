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


#ifndef HAVE_CFG_H
#define HAVE_CFG_H

/* some buffersize definitions */
#define CFG_BUFSIZE 4096	/* max length of one line */
#define CFG_MAX_OPTION 32	/* max length of any option name */
#define CFG_MAX_VALUE 4064	/* max length of any options value */
#define CFG_MAX_FILENAME 256	/* max length of a filename */
#define CFG_VALUES 16		/* max # of arguments an option takes */
#define CFG_MODULES 64		/* max # of dynamically loadable modules */

#define CFG_INCLUDEPATH_ENV "DC_INCLUDEPATH"

/* constants for type of option */
#define ARG_TOGGLE    0		/* TOGGLE on,off; yes,no; 1, 0; */
#define ARG_INT       1		/* callback wants an integer */
#define ARG_STR       2		/* callback expects a \0 terminated str */
#define ARG_LIST      3		/* wants list of strings */
#define ARG_NAME      4		/* wants option name */
#define ARG_RAW       5		/* wants raw argument data */
#define ARG_NONE      6		/* does not expect ANY args */

/* for convenience of terminating the config_options list */
#define LAST_OPTION 	{ "", 0, NULL, 0 }

typedef struct _cfgoption {
	char name[CFG_MAX_OPTION];	/* name of configuration option */
	int type;		/* for possible values, see above */
	void (*callback) ();	/* callback function */
	int userdata;		/* userdefinable value/flag */
} config_option;

/* general configuration items */
struct config {
	/* debug level */
	unsigned int debug;
	/* enable recv.log */
	unsigned int recvlog:1;
	/* dont load modules on startup */
	unsigned int modnoload:1;
	/* dont output anything on start */
	unsigned int quiet:1;
	/* dont detach into background */
	unsigned int foreground:1;
} config;

/* config_read takes the following arguments:
 * 1. the filename of the configuration file to read
 * 2. the list of optionnames to recognize
 *
 * returns 0 on success; !0 on error */
int config_read (char *, config_option *);
void config_register_options (config_option *);

#endif /* HAVE_CFG_H */
