/* NeoStats - IRC Statistical Services 
** Copyright (c) 1999-2004 Adam Rutter, Justin Hammond, Mark Hetherington
** http://www.neostats.net/
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
** $Id: dns.h 1719 2004-04-06 10:41:25Z Fish $
*/

#ifndef _DNS_H_
#define _DNS_H_
#include "adns.h"
#include "serversock.h"

int init_dns (void);
void do_dns (void);
void fini_adns();
void canx_dns(const char *modname);
void do_dns_stats_Z();
int dns_lookup (char *str, adns_rrtype type, void (*callback) (void *data, adns_answer * a), void *data);
void do_reverse_lookup(mqsock *mqs);
void setup_dns_socks();

#endif /* _DNS_H_ */
