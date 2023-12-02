noinst_LIBRARIES = libpopt.a

libpopt_a_SOURCES = \
  lookup3.c \
  popt.c \
  poptconfig.c \
  popt.h \
  popthelp.c \
  poptint.c \
  poptint.h \
  poptparse.c \
  system.h

AM_CPPFLAGS = -DPOPT_SYSCONFDIR="\"$(sysconfdir)\""

EXTRA_DIST  = autogen.sh popt.pdf README.popt
