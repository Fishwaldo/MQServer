bin_PROGRAMS		= mqserver
mqserver_SOURCES 	= main.c serversock.c callback.c client.c dns.c admincli.c libcli.c list.c hash.c conf.c dict.c log.c mythread.c prioq.c \
			  queuemanager.c authqueue.c messagequeue.c
mqserver_LDADD		= -L$(top_srcdir)/lib/src -lmqpacket -L$(top_srcdir)/libevent/ -levent -L$(top_srcdir)/adns/ -ladns -L$(top_srcdir)/confuse/ -lconfuse -lpthread -lcrypt
AM_CFLAGS		= -I$(top_srcdir)/Include -I$(top_srcdir)/adns -I$(top_srcdir)/libevent -I$(top_srcdir)/confuse -DNDEBUG -ggdb 
INCLUDES			= -I$(top_srcdir)/lib/include
include $(top_srcdir)/rules.mk
