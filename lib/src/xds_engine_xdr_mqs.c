/* MQServer - Abstract message Processing Server
** Copyright (c) 2005 Justin Hammond
** http://www.mqserver.info/
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
#include <stdio.h>
#include <string.h>
#include "libmq.h"
#include "packet.h"
#include "xds.h"


extern int
encode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	struct mqpacket *mqp;
	mqp = va_arg (*args, struct mqpacket *);
	return xds_encode (xds, "int32 int32 int32 int32", mqp->outmsg.MID, mqp->outmsg.MSGTYPE, mqp->outmsg.VERSION, mqp->outmsg.flags);
}

extern int
decode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	struct mqpacket *mqp;
	int rc;
	mqp = va_arg (*args, struct mqpacket *);
	rc = xds_decode (xds, "int32 int32 int32 int32", &(mqp->inmsg.MID), &(mqp->inmsg.MSGTYPE), &(mqp->inmsg.VERSION), &(mqp->inmsg.flags));
	return rc;
}
