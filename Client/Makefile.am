bin_PROGRAMS		= testclient testserver
testclient_SOURCES 	= testclient.c list.c hash.c
testclient_LDADD	= -L$(top_srcdir)/Libs/ -lmqpacket
testserver_SOURCES 	= testserver.c list.c hash.c
testserver_LDADD	= -L$(top_srcdir)/Libs/ -lmqpacket
AM_CFLAGS		= -I$(top_srcdir)/Include -I$(top_srcdir)/adns -I$(top_srcdir)/libevent -DNDEBUG

include $(top_srcdir)/rules.mk