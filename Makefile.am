bin_PROGRAMS = lc3as
lc3as_SOURCES = lc3as.c lc3as.h print.c parse.y scan.l popt/popt.h
lc3as_LDADD = popt/libpopt.a

include_HEADERS = lc3.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# TODO get rid of posix yacc warnings?
AM_YFLAGS = -d

SUBDIRS = popt

# TODO move all this under tests with a separate makefile
check_PROGRAMS = test/inst_to_bits test/operations
test_inst_to_bits_SOURCES = test/inst_to_bits.c util.c
test_operations_SOURCES = test/operations.c util.c

check_SCRIPTS = test/all-instructions.sh

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = test/all-instructions.asm

dist_doc_DATA = LICENSE README.md TODO.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS)
