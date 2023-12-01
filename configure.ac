AC_INIT([lc3], m4_esyscmd_s([cat VERSION]), [cliff.snyder@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_LEX([noyywrap])
if test "$LEX" != "flex"; then
    AC_MSG_ERROR([flex not found])
fi
AC_PROG_YACC
if test "$YACC" != "bison -y"; then
    AC_MSG_ERROR([bison not found])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
