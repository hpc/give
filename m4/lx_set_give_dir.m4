AC_DEFUN([X_AC_SET_GIVEDIR], [
  AC_MSG_CHECKING([for the give dir location to use])
  AC_ARG_ENABLE(
    [give-dir],
    AS_HELP_STRING(--enable-give-dir=DIR, specify the give dir location),
    [
      x_ac_give_dir="$enableval"
    ],
    [
      x_ac_give_dir="/usr/give"
    ]
  )
  AC_MSG_RESULT($x_ac_give_dir)
  AC_DEFINE_UNQUOTED([AC_SPOOL_DIRECTORY], $x_ac_give_dir, [The location to use])
])
