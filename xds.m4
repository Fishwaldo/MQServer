dnl ##
dnl ##  OSSP xds - Extensible Data Serialization
dnl ##  Copyright (c) 2001-2003 Ralf S. Engelschall <rse@engelschall.com>
dnl ##  Copyright (c) 2001-2003 The OSSP Project <http://www.ossp.org/>
dnl ##  Copyright (c) 2001-2003 Cable & Wireless Germany <http://www.cw.com/de/>
dnl ##
dnl ##  This file is part of OSSP xds, an extensible data serialization
dnl ##  library which can be found at http://www.ossp.org/pkg/lib/xds/.
dnl ##
dnl ##  Permission to use, copy, modify, and distribute this software for
dnl ##  any purpose with or without fee is hereby granted, provided that
dnl ##  the above copyright notice and this permission notice appear in all
dnl ##  copies.
dnl ##
dnl ##  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED
dnl ##  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
dnl ##  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
dnl ##  IN NO EVENT SHALL THE AUTHORS AND COPYRIGHT HOLDERS AND THEIR
dnl ##  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
dnl ##  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
dnl ##  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
dnl ##  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
dnl ##  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
dnl ##  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
dnl ##  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
dnl ##  SUCH DAMAGE.
dnl ##
dnl ##  xds.m4: XDS-specific autoconf macros
dnl ##

AC_DEFUN([AC_XDS],[
dnl Check for exact data types.
dnl
AC_CHECK_TYPE(u_int8_t, [xds_uint8_t=u_int8_t],
              [AC_CHECK_TYPE(uint8_t, [xds_uint8_t=uint8_t],
			     [AC_MSG_ERROR([no unsigned 8 bit data type found])])])
AC_CHECK_TYPE(u_int16_t, [xds_uint16_t=u_int16_t],
              [AC_CHECK_TYPE(uint16_t, [xds_uint16_t=uint16_t],
			     [AC_MSG_ERROR([no unsigned 16 bit data type found])])])
AC_CHECK_TYPE(u_int32_t, [xds_uint32_t=u_int32_t],
              [AC_CHECK_TYPE(uint32_t, [xds_uint32_t=uint32_t],
			     [AC_MSG_ERROR([no unsigned 32 bit data type found])])])
AC_CHECK_TYPE(u_int64_t, [xds_uint64_t=u_int64_t],
              [AC_CHECK_TYPE(uint64_t, [xds_uint64_t=uint64_t],
			     [AC_MSG_WARN([no unsigned 64 bit data type found])
			     xds_uint64_t=undefined])])
dnl
AC_CHECK_TYPE(int8_t, [xds_int8_t=int8_t],
              [AC_MSG_ERROR([no signed 8 bit data type found])
	      ])
AC_CHECK_TYPE(int16_t, [xds_int16_t=int16_t],
              [AC_MSG_ERROR([no signed 16 bit data type found])
	      ])
AC_CHECK_TYPE(int32_t, [xds_int32_t=int32_t],
              [AC_MSG_ERROR([no signed 32 bit data type found])
	      ])
AC_CHECK_TYPE(int64_t, [xds_int64_t=int64_t],
              [AC_MSG_WARN([no signed 64 bit data type found])
	      xds_int64_t=undefined])
dnl
AC_SUBST([xds_uint8_t])
AC_SUBST([xds_uint16_t])
AC_SUBST([xds_uint32_t])
AC_SUBST([xds_uint64_t])
AC_SUBST([xds_int8_t])
AC_SUBST([xds_int16_t])
AC_SUBST([xds_int32_t])
AC_SUBST([xds_int64_t])
if test "$xds_int64_t" = "undefined" -o "$xds_uint64_t" = "undefined"; then
   have_64_bit_support="#undef XDS_HAVE_64_BIT_SUPPORT"
else
   have_64_bit_support="#define XDS_HAVE_64_BIT_SUPPORT"
fi
AC_SUBST([have_64_bit_support])
])

