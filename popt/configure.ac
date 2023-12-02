AC_INIT([popt], [1.19], [cliff.snyder@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_USE_SYSTEM_EXTENSIONS

AC_CHECK_HEADERS([fnmatch.h glob.h langinfo.h libintl.h mcheck.h stdalign.h])
AC_CHECK_FUNCS([getuid geteuid iconv mtrace secure_getenv __secure_getenv setreuid setuid stpcpy strerror vasprintf srandom glob_pattern_p mbsrtowcs])

AC_PROG_CC
AM_PROG_AR
AC_PROG_RANLIB
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
