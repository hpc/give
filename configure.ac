AC_INIT([give],[3.0],[github-hpc@lanl.gov],[give],[https://www.git.lanl.gov/filesystems/give])

# Keep artifacts in directories as much as possible.
AC_CONFIG_AUX_DIR(auxdir)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall no-define])
AC_CONFIG_HEADERS([config.h])

# Check for compilers.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

# Pull in libtool. We need to support ancient autotools, so don't use LT_INIT.
#LT_INIT
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# Checks for header files.
AC_CHECK_HEADERS([limits.h paths.h stdint.h stdlib.h string.h sys/param.h sys/time.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove mkdir strchr strcspn strdup strspn strtoul wcsdup])

# Check all GIDs for a match instead of just the first one.
X_AC_CHECK_ALL_GIDS

# Check to see if we should only modify the UID permissions.
X_AC_UID_PERM_ONLY

# Allow specifying the give directory.
X_AC_SET_GIVEDIR

echo
echo "========================================================"
echo "==          give: final build configuration           =="
echo "========================================================"
echo "Build Options:"
echo "  Only modify UID permissions ................. $x_ac_uid_perm_only"
echo "  Check all user GIDs ......................... $x_ac_check_all_gids"
echo "  Using give dir location ..................... $x_ac_give_dir"
echo "========================================================"
echo

AC_CONFIG_FILES([ \
    Makefile               \
    src/Makefile           \
    src/string_m/Makefile  \
    doc/Makefile           \
    doc/man/Makefile
])

AC_OUTPUT
