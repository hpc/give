AC_DEFUN([X_AC_SET_STRICT_CHECKS], [
  AC_MSG_CHECKING([if you want to disable strict user checking ie. check for uid == def. gid])
  AC_ARG_ENABLE(
    [non-strict-checks],
    AS_HELP_STRING(--enable-non-strict-checks, stop checking for uid == def gid),
    [
      x_ac_set_strict_checks=no
      AC_DEFINE([STRICT_CHECKING], 0, [Non-strict checking])
      AC_SUBST(STRICT_CHECKS, [0])
    ],
    [
      x_ac_set_strict_checks=yes
      AC_DEFINE([STRICT_CHECKING], 1, [Strict checking]) 
      AC_SUBST(STRICT_CHECKS,[1])
    ]
  )
])

