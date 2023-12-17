bin_PROGRAMS = lc3as lc3vm lc3diff

lc3as_SOURCES =   \
    lc3as.c       \
    parse.h       \
    parse.y       \
    print.c       \
    program.c     \
    program.h     \
    scan.l        \
    popt/popt.h
lc3as_LDADD = popt/libpopt.a

lc3vm_SOURCES =   \
    lc3vm.c       \
    execute.c     \
    interactive.c \
    parse.h       \
    parse.y       \
    program.c     \
    program.h     \
    scan.l        \
    popt/popt.h
lc3vm_LDADD = popt/libpopt.a

lc3diff_SOURCES = lc3diff.c

BUILT_SOURCES = parse.h

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = subdir-objects

AM_YFLAGS = -d -Wno-yacc -Wcounterexamples

SUBDIRS = popt

# TODO move all this under tests with a separate makefile
# check_PROGRAMS =

check_SCRIPTS = \
    test/hello.run.test          \
    test/2048.asm.test           \
    test/rogue.asm.test          \
    test/2048.pretty.test        \
    test/rogue.pretty.test       \
    test/hello.interactive.test  \
    test/all-instructions.pretty.test
MEMCHECKS = \
    test/valgrind-interactive.sh \
    test/valgrind-lc3as-2048.sh  \
    test/valgrind-lc3vm-hello.sh

# uncomment to run memory leak checks (SLOW)
# check_SCRIPTS += $(MEMCHECKS)

CLEANFILES = test/*.valgrind test/*.out

TESTS_ENVIRONMENT = SRCDIR=$(srcdir) BUILDDIR=$(builddir)
TESTS = $(check_PROGRAMS) $(check_SCRIPTS)
TEST_INPUTS = \
    test/2048.asm  test/2048.obj  test/2048.sym  \
    test/rogue.asm test/rogue.obj test/rogue.sym \
    test/hello.asm test/hello.obj test/hello.sym \
    test/all-instructions.asm

TEST_OUTPUTS = test/hello.interactive.out

dist_doc_DATA = LICENSE README.md TODO.md

EXTRA_DIST = $(check_SCRIPTS) $(TEST_INPUTS) $(TEST_OUTPUTS)
