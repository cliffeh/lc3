AC_INIT([lc3], m4_esyscmd_s([cat VERSION]), [cliff.snyder@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_CHECK_PROGS([LEX], [flex])
if test -z "$LEX"; then
   AC_MSG_ERROR([flex not found])
fi
AC_CHECK_PROGS([YACC], [bison])
if test -z "$YACC"; then
   AC_MSG_ERROR([bison not found])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([popt])

AC_OUTPUT
