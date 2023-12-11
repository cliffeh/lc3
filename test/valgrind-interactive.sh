#!/bin/bash
set -euxo pipefail

which valgrind >& /dev/null || { echo "can't find valgrind; skipping test..."; exit 77; }

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

logfile="$BUILDDIR/test/valgrind-interactive.valgrind"

CMDS="assemble $SRCDIR/test/hello.asm\nrun"

echo "$CMDS" | valgrind --leak-check=full --error-exitcode=1 --log-file="$logfile" "$BUILDDIR/lc3vm" -i > /dev/null
