#############################################################################
# Give & Take
#

TOP= .

CC = gcc -Wall -Wextra -Wshadow -std=c99 
CFLAGS= -static
LDFLAGS= -L${TOP}/string_m/
LIBS=-l_str_m_v2



INCLUDES = -I${TOP}/ -I${TOP}/string_m/

CLEANFILES = core *.o *.out *.a *.c~ *.rpm

OBJS =  give-assist.o
PROGS = give-assist give take

all:	${PROGS}


give-assist: give-assist.o 
	${CC} -o $@ give-assist.o ${LDFLAGS} ${LIBS}
	/usr/bin/strip $@

give: give.py
	cp give.py give

take: give.py
	cp give.py take

give-assist.o: give-assist.c config.h
	cd string_m/ && $(MAKE)
	${CC} ${CFLAGS} ${INCLUDES} -c give-assist.c

install: all
	cp give-assist $(bindir)	
	chmod 4555 $(bindir)/give-assist
	cp give $(bindir)
	cp take $(bindir)
	cp give.1 $(mandir)/man1
	chmod 644 $(mandir)/man1/give.1
	cp take.1 $(mandir)/man1
	chmod 644 $(mandir)/man1/take.1

clean:
	cd string_m/ && $(MAKE) $@
	rm -f ${PROGS} ${CLEANFILES} *.tar.bz2

test:
	@echo No test.

check:
	splint ${INCLUDES} +posixlib -compdef give-assist.c
check-some:
	splint ${INCLUDES} +posixlib -nullpass -compdef -nullderef give-assist.c

root: give-assist
	chmod 4555 give-assist




