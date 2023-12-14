#!/bin/bash
set -euxo pipefail

# TODO maybe do this with hexdump to give some more information about what went sideways
# which hexdump >& /dev/null || { echo "can't find hexdump; skipping test..."; exit 77; }

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

"$BUILDDIR/lc3as" -Fpretty "$SRCDIR/test/rogue.asm" -o- | "$BUILDDIR/lc3as" | diff "$SRCDIR/test/rogue.obj" -
