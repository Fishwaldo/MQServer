lib_LTLIBRARIES = libconfuse.la

libconfuse_la_SOURCES = confuse.c lexer.l
include_HEADERS = confuse.h

DEFS = -DLOCALEDIR=\"$(localedir)\" @DEFS@

LIBS = @LIBS@

CLEANFILES=*~ \#*\#
AM_LFLAGS=-Pcfg_yy -olex.yy.c
lexer.c: lexer.l

celan: clean
include $(top_srcdir)/rules.mk