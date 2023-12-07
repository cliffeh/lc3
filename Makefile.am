bin_PROGRAMS = lc3as lc3vm lc3diff
lc3as_SOURCES = scanner.l parser.y lc3as.c lc3as.h util.c util.h popt/popt.h
lc3as_LDADD = popt/libpopt.a

lc3vm_SOURCES = lc3vm.c util.c util.h popt/popt.h
lc3vm_LDADD = popt/libpopt.a

lc3diff_SOURCES = lc3diff.c

include_HEADERS = lc3.h
noinst_HEADERS = lc3as.h util.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_LFLAGS = --header-file=scanner.h
AM_YFLAGS = -d -Wno-yacc

SUBDIRS = popt

# TODO move all this under tests with a separate makefile
check_PROGRAMS = test/inst_to_bits test/unescape_string
test_inst_to_bits_SOURCES = test/inst_to_bits.c util.c util.h
test_unescape_string_SOURCES = test/unescape_string.c util.c util.h

check_SCRIPTS = test/2048.sh test/rogue.sh test/all-instructions.sh test/hello-world.sh
MEMCHECKS = test/valgrind-lc3as-2048.sh test/valgrind-lc3vm-hello-world.sh
check_SCRIPTS += $(MEMCHECKS)

CLEANFILES = test/*.valgrind

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = test/2048.asm test/2048.obj test/rogue.asm test/rogue.obj test/all-instructions.asm test/hello-world.asm test/hello-world.obj

dist_doc_DATA = LICENSE README.md TODO.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS)
