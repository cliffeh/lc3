bin_PROGRAMS = lc3as
lc3as_SOURCES = parse.y scan.l lc3as.c lc3as.h

include_HEADERS = lc3.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_YFLAGS = -d --debug

# TODO move all this under tests with a separate makefile
check_PROGRAMS = test/inst_to_bits test/operations
test_inst_to_bits_SOURCES = test/inst_to_bits.c util.c
test_operations_SOURCES = test/operations.c util.c

check_SCRIPTS = test/all-instructions.sh

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = test/all-instructions.asm

dist_doc_DATA = LICENSE README.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS)
