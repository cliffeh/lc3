#!/bin/bash
set -euxo pipefail

which valgrind >& /dev/null || { echo "can't find valgrind; skipping test..."; exit 77; }

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

logfile="$BUILDDIR/test/valgrind-lc3as-2048.valgrind"

valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 --log-file="$logfile" "$BUILDDIR/lc3as" -Fdebug -o/dev/null "$SRCDIR/test/2048.asm"
