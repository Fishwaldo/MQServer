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


#ifndef _conf_h_
#define _conf_h_

#include "mythread.h"

struct config {
	mylocks configmutex;
	char hostname[MAXHOST];
	int quiet;
	int debug;
	int foreground;
	int xmlport;
	int xdrport;
	int adminport;
} config;



#define GETCONF(x, y) MYLOCK(&config.configmutex); \
 		      x = config.y; \
 		      MYUNLOCK(&config.configmutex);


#endif
