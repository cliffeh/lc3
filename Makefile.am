bin_PROGRAMS = lc3as lc3vm
lc3as_SOURCES = lc3as.c lc3as.h util.c parse.y scan.l popt/popt.h
lc3as_LDADD = popt/libpopt.a

lc3vm_SOURCES = lc3vm.c util.c

include_HEADERS = lc3.h
noinst_HEADERS = lc3as.h util.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# TODO get rid of posix yacc warnings?
AM_YFLAGS = -d

SUBDIRS = popt

# TODO move all this under tests with a separate makefile
check_PROGRAMS = test/inst_to_bits test/operations
test_inst_to_bits_SOURCES = test/inst_to_bits.c util.c util.h
test_operations_SOURCES = test/operations.c util.c util.h

check_SCRIPTS = test/all-instructions.sh test/hello-world.sh

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = test/all-instructions.asm

dist_doc_DATA = LICENSE README.md TODO.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS)
