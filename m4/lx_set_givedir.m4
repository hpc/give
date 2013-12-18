AC_DEFUN([X_AC_SET_GIVEDIR], [
  AC_MSG_CHECKING([where is the givedir, if none is specified /usr/givedir is used])
  AC_ARG_ENABLE(
    [givedir],
    AS_HELP_STRING(--enable-givedir[=DIR], set givedir to value other than /usr/givedir),
    [
      x_ac_givedir="$enableval"
      AC_MSG_RESULT($x_ac_givedir)
      AC_DEFINE_UNQUOTED([SPOOL_DIRECTORY], ["$x_ac_givedir"], [The location to use])
      AC_SUBST(SPOOL, ["$x_ac_givedir"])
    ],
    [
      x_ac_givedir="/usr/givedir"
      AC_MSG_RESULT($x_ac_givedir)
      AC_DEFINE_UNQUOTED([SPOOL_DIRECTORY], ["$x_ac_givedir"], [default givedir]) 
      AC_SUBST(SPOOL,["$x_ac_givedir"])
    ]
  )
])

