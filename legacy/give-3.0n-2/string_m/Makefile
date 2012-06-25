CC=gcc
CFLAGS=-g -Wall -std=c99

OBJS=	getstr_m.o makestr.o sprintf_m.o strcat_m.o strcpy_m.o strdelete_m.o strdup_m.o strlen_m.o strtest.o strtok_m.o strslice_m.o strspn_m.o constraint.o strcmp_m.o  fprintf_m.o fscanf_m.o printf_m.o sscanf_m.o

all:  lib_str_m_v2.a test

depend:
	gccmakedep

lib_str_m_v2.a:	$(OBJS) 
	$(AR) rcs $@ $(OBJS)

test:   lib_str_m_v2.a test.o
	$(CC) -o test test.o -L ./ -l_str_m_v2 

clean:
	-rm -f $(OBJS) test.o test lib_str_m_v2.a
