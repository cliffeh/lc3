bin_PROGRAMS = lc3c
lc3c_SOURCES = scan.l

include_HEADERS = lc3.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

# TODO move all this under tests with a separate makefile
check_PROGRAMS = tests/inst_to_bits tests/operations
tests_inst_to_bits_SOURCES = tests/inst_to_bits.c util.c
tests_operations_SOURCES = tests/operations.c util.c

TESTS = $(check_PROGRAMS)

EXTRA_DIST = README.md LICENSE
