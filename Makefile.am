bin_PROGRAMS = lc3as lc3vm lc3diff
lc3as_SOURCES = parse.y scan.l lc3.h lc3as.c lc3as.h assemble.c print.c popt/popt.h
lc3as_LDADD = popt/libpopt.a

lc3vm_SOURCES = lc3vm.c lc3as.h lc3.h assemble.c execute.c parse.y scan.l popt/popt.h
lc3vm_LDADD = popt/libpopt.a

lc3diff_SOURCES = lc3diff.c

include_HEADERS = lc3.h
noinst_HEADERS = lc3as.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_YFLAGS = -d -Wno-yacc

SUBDIRS = popt

# TODO move all this under tests with a separate makefile
# check_PROGRAMS =

check_SCRIPTS = \
    test/hello.sh \
    test/2048.sh \
    test/rogue.sh \
    test/all-instructions.sh \
    test/interactive-hello.sh
MEMCHECKS = test/valgrind-lc3as-2048.sh test/valgrind-lc3vm-hello.sh test/valgrind-interactive.sh
# uncomment to run memory leak checks (SLOW)
# check_SCRIPTS += $(MEMCHECKS)

CLEANFILES = test/*.valgrind

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = \
    test/2048.asm test/2048.obj \
    test/rogue.asm test/rogue.obj \
    test/hello.asm test/hello.obj \
    test/all-instructions.asm

TEST_OUTPUTS = test/interactive-hello.out

dist_doc_DATA = LICENSE README.md TODO.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS) $(TEST_OUTPUTS)
