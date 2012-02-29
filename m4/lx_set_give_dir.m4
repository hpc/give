AC_DEFUN([X_AC_SET_GIVEDIR], [
  AC_MSG_CHECKING([for the give dir location to use])
  AC_ARG_WITH(
    [give-dir],
    AS_HELP_STRING(--with-give-dir=DIR, specify the give dir location),
    [
      x_ac_with_give_dir="$withval"
    ],
    [
      x_ac_with_give_dir="/usr/give"
    ]
  )
  AC_MSG_RESULT($x_ac_with_give_dir)
])
