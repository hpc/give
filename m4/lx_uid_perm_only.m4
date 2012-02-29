AC_DEFUN([X_AC_UID_PERM_ONLY], [
  AC_MSG_CHECKING([if we should only modify UID permissions])
  AC_ARG_ENABLE(
    [uid-perm-only],
    AS_HELP_STRING(--enable-uid-perm-only, only modify uid permissions),
    [
      x_ac_uid_perm_only=yes
    ],
    [
      x_ac_uid_perm_only=no
    ]
  )
  AC_MSG_RESULT($x_ac_uid_perm_only)
])
