
INCLUDES = -I.. -I../Include/

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

