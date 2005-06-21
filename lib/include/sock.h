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

#ifndef SOCK_H
#define SOCK_H

#define MQS_S_FLAG_GOTSRVCAP 	0x01
#define MQS_S_FLAG_SENTAUTH	0x02
#define MQS_S_FLAG_CONNECTOK	0x04


#define MQS_S_FLAG_SET_GOTSRVCAP(x) (x->flags |= MQS_S_FLAG_GOTSRVCAP)
#define MQS_S_FLAG_IS_GOTSRVCAP(x) (x->flags & MQS_S_FLAG_GOTSRVCAP)

#define MQS_S_FLAG_SET_SENTAUTH(x) (x->flags = MQS_S_FLAG_SENTAUTH)
#define MQS_S_FLAG_IS_SENTAUTH(x) (x->flags & MQS_S_FLAG_SENTAUTH)

#define MQS_S_FLAG_SET_CONNECTOK(x) (x->flags = MQS_S_FLAG_CONNECTOK)
#define MQS_S_FLAG_IS_CONNECTOK(x) (x->flags & MQS_S_FLAG_CONNECTOK)


#endif