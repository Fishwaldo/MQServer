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

/*
 * conf.h
 * dynamic configuration runtime libary
 */

/* define the config types */

#define CFGSTR   1
#define CFGINT   2
#define CFGFLOAT 3
#define CFGBOOL  4

#define CONFBUFSIZE 256

int GetConf (void **data, int type, const char *item);
int SetConf (void *data, int type, char *item);
int GetDir (char *item, char ***data);
int DelConf (char *item);
int DelRow (char *table, char *row);
int DelTable(char *table);
int SetData (void *data, int type, char *table, char *row, char *field);
int GetTableData (char *table, char ***data);
int GetData (void **data, int type, const char *table, const char *row, const char *field);
void flush_keeper();
#endif
