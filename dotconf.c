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
**  Code portions Borrowed from the dotconf libary. Header follows:
*/

/* dotconf
 * Copyright (C) 1999 Lukas Schröder
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/*
** NeoStats CVS Identification
** $Id$
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include "config.h"
#include "defines.h"
#include "dotconf.h"

static int word_count;		/* no. of option arguments */
static char name[CFG_MAX_OPTION + 1];	/* option name */
static char values[CFG_VALUES][CFG_MAX_VALUE + 1];	/* holds the arguments */
static char dotconf_includepath[CFG_MAX_FILENAME + 1];

char *dotconf_file;		/* filename of current file */
int dotconf_line = 0;		/* line in file we are currently reading */

/* 
 * the list of config_options where a match for each name-token is 
 * searched for; i prefer the use of a fixed-size array, b/c special
 * memory handling (malloc,free) would be necessary
 */
static config_option *config_options[CFG_MODULES];

/*
 * some 'magic' options that are predefined by dot.conf itself for
 * advanced functionality
 */
static void dotconf_cb_include (char *);	/* magic 'Include' */
static void dotconf_cb_includepath (char *);	/* magic 'IncludePath' */

static config_option dotconf_options[] = { {"Include", ARG_STR, dotconf_cb_include, 0},
{"IncludePath", ARG_STR, dotconf_cb_includepath, 0},
LAST_OPTION
};

void
config_substitute_env (char *str)
{
	char *cp1, *cp2, *cp3, *eos, *eob;
	char *env_value;
	char env_name[CFG_MAX_VALUE + 1];
	char env_default[CFG_MAX_VALUE + 1];
	char tmp_value[CFG_MAX_VALUE + 1];

	bzero (env_name, CFG_MAX_VALUE + 1);
	bzero (env_default, CFG_MAX_VALUE + 1);
	bzero (tmp_value, CFG_MAX_VALUE + 1);
	cp1 = str;
	eob = cp1 + CFG_MAX_VALUE + 1;
	cp2 = tmp_value;
	eos = cp2 + CFG_MAX_VALUE + 1;

	while ((cp1 != eos) && (cp2 != eos) && (*cp1 != '\0')) {
		/* substitution needed ?? */
		if (*cp1 == '$' && *(cp1 + 1) == '{') {
			/* yeah */
			cp1 += 2;	/* skip ${ */
			cp3 = env_name;
			while ((cp1 != eos)
			       && !(*cp1 == '}' || *cp1 == ':'))
				*cp3++ = *cp1++;
			*cp3 = '\0';	/* terminate */

			/* default substitution */
			if (*cp1 == ':' && *(cp1 + 1) == '-') {
				cp1 += 2;	/* skip :- */
				cp3 = env_default;
				while ((cp1 != eos) && (*cp1 != '}'))
					*cp3++ = *cp1++;
				*cp3 = '\0';	/* terminate */
			} else
				while ((cp1 != eos) && (*cp1 != '}'))
					cp1++;

			if (*cp1 != '}')
				fprintf (stderr, "%s:%d: Unbalanced '{'\n", dotconf_file, dotconf_line);
			else {
				cp1++;	/* skip } */
				if ((env_value = getenv (env_name)) != NULL) {
					strncat (cp2, env_value, eos - cp2);
					cp2 += strlen (env_value);
				} else {
					strncat (cp2, env_default, eos - cp2);
					cp2 += strlen (env_default);
				}
			}

		}

		*cp2++ = *cp1++;
	}
	*cp2 = '\0';		/* terminate buffer */

	strncpy (str, tmp_value, CFG_MAX_VALUE + 1);
}

void
config_register_options (config_option * options)
{
	int i;
	for (i = 0; i < CFG_MODULES && config_options[i]; i++) {
	}
	config_options[i] = options;
}

int
config_parse (FILE * config)
{
	static char buffer[CFG_BUFSIZE];
	static char *here_string;	/* Damn FreeBSD */
	static char *here_limit;
	static char *here_doc;

	while ((fgets (buffer, CFG_BUFSIZE, config)) != NULL) {	/* for each line */
		char *cp1, *cp2;	/* generic char pointer                          */
		char *eob, *eos;	/* end of buffer; end of string                  */
		char sq, dq;	/* state flags: single/double quote              */
		int i, mod;	/* generic counter, mod holds the module index   */
		config_option opt;	/* matched option from config_options */

		dotconf_line++;

		/* ignore #-comments and empty lines */
		if ((buffer[0] == '#' || buffer[0] == '\n'))
			continue;

		/* clear fields */
		word_count = 0;
		name[0] = 0;
		for (i = 0; i < CFG_VALUES; i++)
			values[i][0] = 0;

		/* initialize char pointer */
		cp1 = buffer;
		eob = cp1 + strlen (cp1);	/* calculate end of buffer */

		/* skip any whitspace of indented lines */
		while ((cp1 != eob) && (isspace (*cp1)))
			cp1++;
		/* skip line if it only contains whitespace */
		if (cp1 == eob)
			continue;

		/* get first token: read the name of a possible option */
		cp2 = name;
		while ((*cp1 != '\0') && (!isspace (*cp1)))
			*cp2++ = *cp1++;
		*cp2 = '\0';

		/* and now find the entry in the option table, and call the callback */
		bzero (&opt, sizeof (config_option));
		for (mod = 0; mod < CFG_MODULES && config_options[mod]; mod++)
			for (i = 0; config_options[mod][i].name[0]; i++)
				if (!strncmp (name, config_options[mod][i].name, CFG_MAX_OPTION)) {
					opt = config_options[mod][i];
					break;	/* found it; break out of for */
				}

		if (opt.name[0] == 0) {
			/* cannot find it 
			   fprintf(stderr, "Unknown Config-Option: '%s' in %s:%d\n",
			   name, dotconf_file, dotconf_line);
			   continue;            move on to the next line immediately */
		} else if (opt.type == ARG_RAW) {
			/* if it is an ARG_RAW type, save some time and call the
			   callback now */
			opt.callback (cp1, opt.userdata);
			continue;
		} else if (opt.type == ARG_STR) {
			/* check if it's a here-document and act accordingly */
			char *cp3 = cp1;

			bzero (&here_limit, 9);

			/* skip whitespace */
			while ((cp3 < eob) && (*cp3 != '\0')
			       && (isspace (*cp3)))
				cp3++;

			if (!strncmp ("<<", cp3, 2)) {	/* here string sign */
				/* it's a here-document: yeah, what a cool feature ;) */
				struct stat finfo;

				stat (dotconf_file, &finfo);
				/* 
				 * allocate a buffer of filesize bytes; should be enough to
				 * prevent buffer overflows
				 */
				here_doc = malloc (finfo.st_size + 1);	/* allocate  buffer memory */
				bzero (here_doc, finfo.st_size + 1);

				strncpy (here_limit, cp3 + 2, 8);	/*   copy here-delimiter */
				while (fgets (buffer, CFG_BUFSIZE, config)) {
					if (!strncmp (here_limit, buffer, strlen (here_limit))) {
						here_string = 0;
						break;
					}
					strcat (here_doc, buffer);	/*     append to  buffer */
				}
				if (here_string)
					fprintf (stderr, "Line %d: Unterminated here-document!\n", dotconf_line);
				here_doc[strlen (here_doc) - 1] = '\0';	/*    strip newline */
				opt.callback (here_doc, opt.userdata);	/* call back */

				free (here_doc);	/*  free buffer memory */

				continue;
			}

		}

		free (here_doc);
		/* skip whitespace */
		while ((cp1 < eob) && (*cp1 != '\0') && (isspace (*cp1)))
			cp1++;

		/* start reading option arguments */
		cp2 = values[word_count];
		eos = cp2 + CFG_MAX_VALUE - 1;
		sq = 0;
		dq = 0;		/* clear quoting flags */
		while ((*cp1 != '\0') && (cp2 != eos)
		       && (word_count < CFG_VALUES)) {
			switch (*cp1) {
			case '\'':	/* single quote */
				if (dq)
					break;	/* already double quoting, break out */
				if (sq) {
					sq--;
				} /* already single quoting, clear state */
				else if (!sq) {
					sq++;
				}	/* set state for single quoting */
				break;
			case '"':	/* double quote */
				if (sq)
					break;	/* already single quoting, break out */
				if (dq) {
					dq--;
				} /* already double quoting, clear state */
				else if (!dq) {
					dq++;
				}	/* set state for double quoting */
				break;
			default:
				break;
			}
			/* unquoted space: start a new option argument */
			if (isspace (*cp1) && !dq && !sq) {
				*cp2 = '\0';	/* terminate current argument */
				/* increment word counter and update char pointer */
				cp2 = values[++word_count];
				/* skip all whitespace between 2 arguments */
				while (isspace (*(cp1 + 1))
				       && (*cp1 != '\0'))
					cp1++;
			}
			/* not space or quoted ; eat it: */
			else if ((((!isspace (*cp1) && !dq && !sq && *cp1 != '"' && *cp1 != '\'')
				   /* dont take quote if quoting: */
				   || (dq && (*cp1 != '"'))
				   || (sq && *cp1 != '\''))))
				*cp2++ = *cp1;
			cp1++;
		}

		if (opt.name[0] > 32) {	/* has an option entry been found before? */
			/* found it, now check the type of args it wants */
#define USER_DATA opt.userdata
			switch (opt.type) {
			case ARG_TOGGLE:
				{
					/* the value is true if the argument is Yes, On or 1 */
					/* kludge code follows ;) */
					int arg = ((values[0][0] == 'Y' || values[0][1] == 'y')
						   || (values[0][0] == '1')
						   || ((values[0][0] == 'o' || values[0][0] == 'O')
						       && (values[0][1] == 'n' || values[0][1]
							   == 'N')));
					opt.callback (arg, USER_DATA);
					break;
				}
			case ARG_INT:
				{
					int arg = atoi (values[0]);
					opt.callback (arg, USER_DATA);
					break;
				}
			case ARG_STR:
				{
					config_substitute_env (values[0]);
					opt.callback (values[0], USER_DATA);
					break;
				}
			case ARG_LIST:
				{
					char *data[CFG_VALUES];
					int i;
					for (i = 0; i < word_count; i++) {	/* prepare list */
						config_substitute_env (values[i]);
						data[i] = strdup (values[i]);
					}
					opt.callback (data, word_count, USER_DATA);

					for (i = 0; i < word_count; i++)	/* dump list */
						free (data[i]);

					break;
				}
			case ARG_NONE:
				{
					opt.callback ();
					break;
				}
			case ARG_RAW:	/* this has been handled before */
			default:
				{
					break;
				}
			}
		}
	}
	return -1;
}

/*
 * open and parse the config-file using the config_options list
 * as reference for what callback to call and what type of arguments to provide
 */
int
config_read (char *fname, config_option * options)
{
	FILE *config;
	char *dc_env;		/* pointer to DC_INCLUDEPATH */

	if (access (fname, R_OK)) {
		fprintf (stderr, "Error opening configuration file '%s'\n", fname);
		return 1;
	}

	dotconf_file = malloc (CFG_MAX_FILENAME + 1);	/* allocate fname buffer */
	bzero (dotconf_file, CFG_MAX_FILENAME + 1);
	bzero (dotconf_includepath, CFG_MAX_FILENAME + 1);

	strncpy (dotconf_file, fname, CFG_MAX_FILENAME);	/* fill fname buffer */

	/* take includepath from environment if present */
	if ((dc_env = getenv (CFG_INCLUDEPATH_ENV)) != NULL)
		strncpy (dotconf_includepath, dc_env, CFG_MAX_FILENAME);

	config_register_options (dotconf_options);	/* internal options */
	config_register_options (options);	/* register main options */

	config = fopen (dotconf_file, "r");
	config_parse (config);	/* fire off parser */
	fclose (config);

	free (dotconf_file);	/* free fname buffer */

	return 0;
}

/* callbacks for internal options */

void
dotconf_cb_include (char *str)
{
	FILE *config;
	char old_fname[CFG_MAX_FILENAME];

	bzero (&old_fname, CFG_MAX_FILENAME);
	strncpy (old_fname, dotconf_file, CFG_MAX_FILENAME);
	if (str[0] != '/' && dotconf_includepath[0] != '\0') {
		/* relative file AND include path is used */

		/* check for length of fully qualified filename */
		if ((strlen (str) + strlen (dotconf_includepath) + 1) == CFG_MAX_FILENAME) {
			fprintf (stderr, "%s:%d: Absolute filename too long (>%d)\n", dotconf_file, dotconf_line, CFG_MAX_FILENAME);
			return;
		}

		snprintf (dotconf_file, CFG_MAX_FILENAME + 1, "%s/%s", dotconf_includepath, str);
	} else			/* fully qualified, or no includepath */
		strncpy (dotconf_file, str, CFG_MAX_FILENAME);

	if (access (dotconf_file, R_OK)) {
		fprintf (stderr, "Error in %s line %d: Cannot open %s for inclusion\n", old_fname, dotconf_line, dotconf_file);
		strncpy (dotconf_file, old_fname, CFG_MAX_FILENAME);	/* restore settings */
		return;
	}

	config = fopen (dotconf_file, "r");
	config_parse (config);
	fclose (config);
	strncpy (dotconf_file, old_fname, CFG_MAX_FILENAME);
}

void
dotconf_cb_includepath (char *str)
{
	char *env = getenv ("DC_INCLUDEPATH");
	if (!env)		/* environment overrides configuration file setting */
		strncpy (dotconf_includepath, str, CFG_MAX_FILENAME);
}
