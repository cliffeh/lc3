bin_PROGRAMS = lc3as
lc3as_SOURCES = lc3as.c lc3as.h parse.y scan.l

include_HEADERS = lc3.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_YFLAGS = -d --debug

# TODO move all this under tests with a separate makefile
check_PROGRAMS = tests/inst_to_bits tests/operations
tests_inst_to_bits_SOURCES = tests/inst_to_bits.c util.c
tests_operations_SOURCES = tests/operations.c util.c

TESTS = $(check_PROGRAMS)

EXTRA_DIST = README.md LICENSE
