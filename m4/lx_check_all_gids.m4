AC_DEFUN([X_AC_CHECK_ALL_GIDS], [
  AC_MSG_CHECKING([if we look at all user GIDs for a match])
  AC_ARG_ENABLE(
    [check-all-gids],
    AS_HELP_STRING(--enable-check-all-gids, check all gids for a match),
    [
      x_ac_check_all_gids=yes
      AC_DEFINE([AC_CHECK_ALL_GIDS], 1, [Check all user GIDs for a match])
    ],
    [
      x_ac_check_all_gids=no
    ]
  )
  AC_MSG_RESULT($x_ac_check_all_gids)
])
