bin_PROGRAMS		= mqserver
mqserver_SOURCES 	= main.c serversock.c callback.c client.c dns.c admincli.c libcli.c list.c hash.c conf.c dict.c dotconf.c log.c mythread.c prioq.c
mqserver_LDADD		= -L$(top_srcdir)/Libs/ -lmqpacket -L$(top_srcdir)/libevent/ -levent -L$(top_srcdir)/adns/ -ladns -lpthread -lcrypt
AM_CFLAGS		= -I$(top_srcdir)/Include -I$(top_srcdir)/adns -I$(top_srcdir)/libevent -DNDEBUG -ggdb 

include $(top_srcdir)/rules.mk