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

#include "defines.h"
#include "confuse.h"
#include "conf.h"
#include "log.h"

/* XXX we need to protect this with mutex's */
cfg_t *cfg;

cfg_opt_t main_opts[] = {
	CFG_STR("name", "${HOSTNAME}", CFGF_NONE),
	CFG_BOOL("xml-protocol-enable", cfg_true, CFGF_NONE),
	CFG_INT("xml-protocol-port", 8889, CFGF_NONE),
	CFG_BOOL("xdr-protocol-enable", cfg_true, CFGF_NONE),
	CFG_INT("xdr-protocol-port", 8888, CFGF_NONE),
	CFG_BOOL("telnet-enable", cfg_true, CFGF_NONE),
	CFG_INT("telnet-port", 8887, CFGF_NONE),
	CFG_BOOL("load-quiet", cfg_false, CFGF_NONE),
	CFG_INT("debug-level", 0, CFGF_NONE),
	CFG_BOOL("foreground", cfg_false, CFGF_NONE),
	CFG_END()
};

cfg_opt_t opts[] = {
	CFG_SEC("main", main_opts, CFGF_NONE),
	CFG_END()
};



int
ConfLoad ()
{
	MYLOCK_INIT(config.configmutex);

	MYLOCK(&config.configmutex);
	/* Read in the Config File */
	printf ("Reading the Config File. Please wait.....\n");
	cfg = cfg_init(opts, CFGF_NOCASE);
	if (cfg_parse(cfg, "mqserver.cfg") == CFG_PARSE_ERROR) {
		printf ("***************************************************\n");
		printf ("*                  Error!                         *\n");
		printf ("*                                                 *\n");
		printf ("* Config File not found, or Unable to Open        *\n");
		printf ("* Please check its Location, and try again        *\n");
		printf ("*                                                 *\n");
		printf ("*             NeoStats NOT Started                *\n");
		printf ("***************************************************\n");
		MYUNLOCK(&config.configmutex);
		return NS_FAILURE;
	}

	strncpy(config.hostname, cfg_getstr(cfg, "main|name"), MAXHOST);
	if (cfg_getbool(cfg, "main|xml-protocol-enable") == cfg_true) {
		config.xmlport = cfg_getint(cfg, "main|xml-protocol-port");
	} else {
		config.xmlport = -1;
	}
	if (cfg_getbool(cfg, "main|xdr-protocol-enable") == cfg_true) {
		config.xdrport = cfg_getint(cfg, "main|xdr-protocol-port");
	} else {
		config.xdrport = -1;
	}
	if (cfg_getbool(cfg, "main|telnet-enable") == cfg_true) {
		config.adminport = cfg_getint(cfg, "main|telnet-port");
	} else {
		config.adminport = -1;
	}
	config.quiet = cfg_getbool(cfg, "main|load-quiet");
	config.debug = cfg_getint(cfg, "main|debug-level");
	config.foreground = cfg_getbool(cfg, "main|foreground");
	

	if (!config.quiet) {
		printf ("Sucessfully Loaded Config File, Now Booting %s MQServer\n", cfg_getstr(cfg, "main|name"));
	}
	MYUNLOCK(&config.configmutex);
	return NS_SUCCESS;
}


