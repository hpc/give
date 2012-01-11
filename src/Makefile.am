include $(top_srcdir)/common.mk

SUBDIRS = string_m

bin_PROGRAMS = give-assist
give_assist_SOURCES = give-assist.c
give_assist_LDADD = \
    $(top_srcdir)/src/string_m/lib_string_m.a

give_assist_CPPFLAGS = \
    -I$(top_srcdir)/src/string_m
