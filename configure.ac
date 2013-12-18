#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([give],[3.1],[github-hpc@lanl.gov],[give],[https://www.git.lanl.gov/filesystems/give])

AC_CONFIG_AUX_DIR([auxdir])
AC_CONFIG_MACRO_DIR([m4])

m4_include([m4/lx_set_givedir.m4])
m4_include([m4/lx_set_strict_checks.m4])

AM_INIT_AUTOMAKE([-Wall no-define])

AC_CONFIG_SRCDIR([give-assist.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CXX

AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([limits.h paths.h stdint.h stdlib.h string.h sys/param.h sys/time.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([memmove mkdir strchr strcspn strdup strspn strtoul])

#set a givedir, if none set, /usr/givedir is used
X_AC_SET_GIVEDIR

#set strict checking (uid == default gid)
X_AC_SET_STRICT_CHECKS

echo
echo "========================================================"
echo "==          give: final build configuration           =="
echo "========================================================"
echo "Build Options:"
echo "  Strict checking ............................. $x_ac_set_strict_checks"
echo "  Using give dir location ..................... $x_ac_givedir"
echo "========================================================"
echo

AC_CONFIG_FILES([Makefile
                 give.py
])
AC_OUTPUT
