
INCLUDES = -I..

SRCS = check.c event.c general.c parse.c query.c reply.c setup.c transmit.c types.c
OBJS = ${SRCS:.c=.o}

all: modules

modules: $(OBJS)
	rm -f libadns.a
	ar cru libadns.a $(OBJS)
	ranlib libadns.a

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

clean:
	$(RM) *.o *.lo *.so *.a  *.exe config.h

distclean:
	$(RM) *.o *.lo *.so *.a  *.exe config.h

depend:
	$(CC) -MM $(INCLUDES) $(CFLAGS) $(SRCS) > .depend

install:
	$(INSTALL_DATA) adns.h $(INCDIRECTORY)

DISTFILES =*.c adns.h dlist.h internal.h tvarith.h Makefile *.in

dist:
	@for file in $(DISTFILES); do \
		cp -pr $$file $(DISTDIR)/$$file; \
	done

$(OBJS): Makefile 

check.o: check.c internal.h config.h adns.h dlist.h
event.o: event.c internal.h config.h adns.h dlist.h tvarith.h
general.o: general.c internal.h config.h adns.h dlist.h
parse.o: parse.c internal.h config.h adns.h dlist.h
query.o: query.c internal.h config.h adns.h dlist.h
reply.o: reply.c internal.h config.h adns.h dlist.h
setup.o: setup.c internal.h config.h adns.h dlist.h
transmit.o: transmit.c internal.h config.h adns.h dlist.h tvarith.h
types.o: types.c internal.h config.h adns.h dlist.h