#include <stdio.h>
#include <string.h>
#include "defines.h"
#include "packet.h"
#include "xds.h"


extern int
encode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	struct mqpacket *mqp;
	mqp = va_arg (*args, struct mqpacket *);
printf("encode %d\n", mqp->outmsg.MID);
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
