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
** $Id: log.h 3 2004-04-27 14:19:24Z Fish $
*/

#ifndef _xds_engine_xdr_mqs_h_
#define _xds_engine_xdr_mqs_h_

int decode_mqs_header(xds_t* xds, void* engine_context, void* buffer, size_t buffer_size, size_t* used_buffer_size, va_list* args);
int encode_mqs_header(xds_t* xds, void* engine_context, void* buffer, size_t buffer_size, size_t* used_buffer_size, va_list* args);

#endif
