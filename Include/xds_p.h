/*
**  OSSP xds - Extensible Data Serialization
**  Copyright (c) 2001-2003 Ralf S. Engelschall <rse@engelschall.com>
**  Copyright (c) 2001-2003 The OSSP Project <http://www.ossp.org/>
**  Copyright (c) 2001-2003 Cable & Wireless Germany <http://www.cw.com/de/>
**
**  This file is part of OSSP xds, an extensible data serialization
**  library which can be found at http://www.ossp.org/pkg/lib/xds/.
**
**  Permission to use, copy, modify, and distribute this software for
**  any purpose with or without fee is hereby granted, provided that
**  the above copyright notice and this permission notice appear in all
**  copies.
**
**  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
**  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
**  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
**  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
**  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
**  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
**  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
**  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
**  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
**  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
**  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
**  SUCH DAMAGE.
**
**  xds_p.h: internal library API
*/

#ifndef __XDS_P_H__
#define __XDS_P_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xds.h"

#define XDS_INITIAL_BUFFER_CAPACITY   512
#define XDS_INITIAL_ENGINES_CAPACITY  32

typedef struct {
    char *name;
    xds_engine_t engine;
    void *context;
} engine_map_t;

struct xds_context {
    xds_mode_t mode;
    char *buffer;
    size_t buffer_len;
    size_t buffer_capacity;
    int we_own_buffer;
    engine_map_t *engines;
    size_t engines_len;
    size_t engines_capacity;
};

#endif /* __XDS_P_H__ */
