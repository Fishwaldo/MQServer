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
	return xds_encode (xds, "uint32 uint32 uint32 uint32", mqp->MID, mqp->MSGTYPE, mqp->VERSION, mqp->flags);
}

extern int
decode_mqs_header (xds_t * xds, void *engine_context, void *buffer, size_t buffer_size, size_t * used_buffer_size, va_list * args)
{
	struct mqpacket *mqp;
	int rc;
	mqp = va_arg (*args, struct mqpacket *);
	rc = xds_decode (xds, "uint32 uint32 uint32 uint32", &(mqp->MID), &(mqp->MSGTYPE), &(mqp->VERSION), &(mqp->flags));
	return rc;
}
