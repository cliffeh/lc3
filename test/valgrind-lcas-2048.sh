#!/bin/bash
set -euxo pipefail

which valgrind >& /dev/null || { echo "can't find valgrind; skipping test..."; exit 77; }

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

logfile="$BUILDDIR/test/valgrind-lcas-2048.valgrind"

valgrind --leak-check=full --log-file="$logfile" "$BUILDDIR/lc3as" "$SRCDIR/test/2048.asm" > /dev/null
cat "${logfile}"
grep 'ERROR SUMMARY: 0' "$logfile" || { echo "leak detected; exiting..."; exit 1; }
