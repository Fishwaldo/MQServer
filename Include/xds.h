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
**  xds.h: XDS library API
*/

#ifndef __XDS_H__
#define __XDS_H__

#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <assert.h>

#define XDS_TRUE  (1==1)
#define XDS_FALSE (1!=1)

#define XDS_HAVE_64_BIT_SUPPORT

typedef u_int8_t  xds_uint8_t;
typedef int8_t   xds_int8_t;
typedef u_int16_t xds_uint16_t;
typedef int16_t  xds_int16_t;
typedef u_int32_t xds_uint32_t;
typedef int32_t  xds_int32_t;
#ifdef XDS_HAVE_64_BIT_SUPPORT
typedef u_int64_t xds_uint64_t;
typedef int64_t  xds_int64_t;
#endif
typedef float          xds_float_t;
typedef double         xds_double_t;

enum {
    XDS_OK                 =  0,
    XDS_ERR_NO_MEM         = -1,
    XDS_ERR_OVERFLOW       = -2,
    XDS_ERR_INVALID_ARG    = -3,
    XDS_ERR_TYPE_MISMATCH  = -4,
    XDS_ERR_UNKNOWN_ENGINE = -5,
    XDS_ERR_INVALID_MODE   = -6,
    XDS_ERR_UNDERFLOW      = -7,
    XDS_ERR_UNKNOWN        = -8,
    XDS_ERR_SYSTEM         = -9
};

typedef enum { XDS_ENCODE, XDS_DECODE } xds_mode_t;
typedef enum { XDS_LOAN,   XDS_GIFT   } xds_scope_t;

struct xds_context;
typedef struct xds_context xds_t;

typedef int (*xds_engine_t)(xds_t *xds, void *engine_context,
                            void *buffer, size_t buffer_size, size_t *used_buffer_size,
                            va_list *args);

int    xds_init      (xds_t **xds, xds_mode_t);
int    xds_destroy   (xds_t *xds);
int    xds_register  (xds_t *xds, const char *name, xds_engine_t engine, void *engine_context);
int    xds_unregister(xds_t *xds, const char *name);
int    xds_setbuffer (xds_t *xds, xds_scope_t flag, void *buffer, size_t buffer_len);
int    xds_getbuffer (xds_t *xds, xds_scope_t flag, void **buffer, size_t *buffer_len);
int    xds_encode    (xds_t *xds, const char *fmt, ...);
int    xds_decode    (xds_t *xds, const char *fmt, ...);
int    xds_vencode   (xds_t *xds, const char *fmt, va_list args);
int    xds_vdecode   (xds_t *xds, const char *fmt, va_list args);

#define xds_check_parameter(condition) \
    do { \
        assert(condition); \
        if (!(condition)) \
            return XDS_ERR_INVALID_ARG; \
    } while (XDS_FALSE)

#define xds_init_encoding_engine(size) \
    do { \
        xds_check_parameter(xds != NULL); \
        xds_check_parameter(buffer != NULL); \
        xds_check_parameter(buffer_size != 0); \
        xds_check_parameter(used_buffer_size != NULL && *used_buffer_size == 0); \
        xds_check_parameter(args != NULL); \
        *used_buffer_size = size; \
        if (buffer_size < size) \
            return XDS_ERR_OVERFLOW; \
    } while (XDS_FALSE)

#define xds_init_decoding_engine(size) \
    do { \
        xds_check_parameter(xds != NULL); \
        xds_check_parameter(buffer != NULL); \
        xds_check_parameter(buffer_size != 0); \
        xds_check_parameter(used_buffer_size != NULL && *used_buffer_size == 0); \
        xds_check_parameter(args != NULL); \
        *used_buffer_size = size; \
        if (buffer_size < size) \
            return XDS_ERR_UNDERFLOW; \
    } while (XDS_FALSE)

#define xds_declare_formatting_engine(func) \
    int func(xds_t *, void *, void *, size_t, size_t *, va_list *)

xds_declare_formatting_engine(xdr_encode_uint32);
xds_declare_formatting_engine(xdr_decode_uint32);
xds_declare_formatting_engine(xdr_encode_int32);
xds_declare_formatting_engine(xdr_decode_int32);
#ifdef XDS_HAVE_64_BIT_SUPPORT
xds_declare_formatting_engine(xdr_encode_uint64);
xds_declare_formatting_engine(xdr_decode_uint64);
xds_declare_formatting_engine(xdr_encode_int64);
xds_declare_formatting_engine(xdr_decode_int64);
#endif
xds_declare_formatting_engine(xdr_encode_float);
xds_declare_formatting_engine(xdr_decode_float);
xds_declare_formatting_engine(xdr_encode_double);
xds_declare_formatting_engine(xdr_decode_double);
xds_declare_formatting_engine(xdr_encode_octetstream);
xds_declare_formatting_engine(xdr_decode_octetstream);
xds_declare_formatting_engine(xdr_encode_string);
xds_declare_formatting_engine(xdr_decode_string);

xds_declare_formatting_engine(xml_encode_begin);
xds_declare_formatting_engine(xml_decode_begin);
xds_declare_formatting_engine(xml_encode_end);
xds_declare_formatting_engine(xml_decode_end);
xds_declare_formatting_engine(xml_encode_uint32);
xds_declare_formatting_engine(xml_decode_uint32);
xds_declare_formatting_engine(xml_encode_int32);
xds_declare_formatting_engine(xml_decode_int32);
#ifdef XDS_HAVE_64_BIT_SUPPORT
xds_declare_formatting_engine(xml_encode_uint64);
xds_declare_formatting_engine(xml_decode_uint64);
xds_declare_formatting_engine(xml_encode_int64);
xds_declare_formatting_engine(xml_decode_int64);
#endif
xds_declare_formatting_engine(xml_encode_float);
xds_declare_formatting_engine(xml_decode_float);
xds_declare_formatting_engine(xml_encode_double);
xds_declare_formatting_engine(xml_decode_double);
xds_declare_formatting_engine(xml_encode_octetstream);
xds_declare_formatting_engine(xml_decode_octetstream);
xds_declare_formatting_engine(xml_encode_string);
xds_declare_formatting_engine(xml_decode_string);

int xds_get_usedbuffer (xds_t * xds);


#endif /* __XDS_H__ */

