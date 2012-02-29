AC_DEFUN([X_AC_SPECIFY_GID], [
  AC_MSG_CHECKING([if we should use a specific GID integer while giving])
  AC_ARG_ENABLE(
    [use-gid],
    AS_HELP_STRING(--enable-use-gid=GID_INT, always use a specific GID integer while giving),
    [
      x_ac_specify_gid=$enableval
      AC_DEFINE_UNQUOTED([AC_SPECIFIC_GID], $x_ac_specify_gid, [Always use a specific GID integer while giving])
    ],
    [
      x_ac_specify_gid="gid of uid only"
    ]
  )
  AC_MSG_RESULT($x_ac_specify_gid)
])
