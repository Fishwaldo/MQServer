bin_PROGRAMS		= testclient
testclient_SOURCES 	= testclient.c list.c hash.c
testclient_LDADD	= -L$(top_srcdir)/Libs/ -lmqpacket -lreadline -lncurses
AM_CFLAGS		= -I$(top_srcdir)/Include -I$(top_srcdir)/adns -I$(top_srcdir)/libevent -DNDEBUG

include $(top_srcdir)/rules.mk
