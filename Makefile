#############################################################################
# Give & Take
#
TOP= .

CC = gcc -Wall
# getpwXXX_r() routines spoil static
CFLAGS= -static
LDFLAGS= -L${TOP}/string_m/
LIBS=-l_str_m_v2
TARGET=/usr/bin
TARGETMAN=/usr/share/man/man1


INCLUDES = -I${TOP}/ -I${TOP}/string_m/

CLEANFILES = core *.o *.out *.a *.c~ *.rpm

OBJS =  give-assist.o
PROGS = give-assist give take

.c.o:
	${CC} ${CFLAGS} ${INCLUDES} -c $<

all:	${PROGS}

give-assist: give-assist.o
	${CC} -o $@ give-assist.o ${LDFLAGS} ${LIBS}
	/usr/bin/strip $@

give: give.py
	cp give.py give

take: give.py
	cp give.py take

install: all
	cp give-assist $(TARGET)
	chown root:root $(TARGET)/give-assist
	chmod 4755 $(TARGET)/give-assist
	cp give $(TARGET)
	cp take $(TARGET)
	cp give.1 $(TARGETMAN)
	chmod 644 $(TARGETMAN)/give.1
	cp take.1 $(TARGETMAN)
	chmod 644 $(TARGETMAN)/take.1

clean:
	rm -f ${PROGS} ${CLEANFILES} *.tar.bz2

test:
	@echo No test.

check:
	splint ${INCLUDES} +posixlib -compdef give-assist.c
check-some:
	splint ${INCLUDES} +posixlib -nullpass -compdef -nullderef give-assist.c

root: give-assist
	chown root:root give-assist
	chmod 4755 give-assist

rpm: install
	cat installed-manifest | make-rpm givetake 3.0 1 '' '' '' rpm-config.xml
