/*
**  xds_version.c -- Version Information for OSSP xds (syntax: C/C++)
**  [automatically generated and maintained by GNU shtool]
*/

#ifdef _XDS_VERSION_C_AS_HEADER_

#ifndef _XDS_VERSION_C_
#define _XDS_VERSION_C_

#define XDS_VERSION 0x009200

typedef struct {
	const int v_hex;
	const char *v_short;
	const char *v_long;
	const char *v_tex;
	const char *v_gnu;
	const char *v_web;
	const char *v_sccs;
	const char *v_rcs;
} xds_version_t;

extern xds_version_t xds_version;

#endif /* _XDS_VERSION_C_ */

#else /* _XDS_VERSION_C_AS_HEADER_ */

#define _XDS_VERSION_C_AS_HEADER_
#include "xds_version.c"
#undef  _XDS_VERSION_C_AS_HEADER_

xds_version_t xds_version = {
	0x009200,
	"0.9.0",
	"0.9.0 (17-Feb-2003)",
	"This is OSSP xds, Version 0.9.0 (17-Feb-2003)",
	"OSSP xds 0.9.0 (17-Feb-2003)",
	"OSSP xds/0.9.0",
	"@(#)OSSP xds 0.9.0 (17-Feb-2003)",
	"$Id: OSSP xds 0.9.0 (17-Feb-2003) $"
};

#endif /* _XDS_VERSION_C_AS_HEADER_ */
