#!/bin/bash
set -euxo pipefail

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

"$BUILDDIR/lc3as" -i "$SRCDIR/test/all-instructions.asm" | diff "$SRCDIR/test/all-instructions.asm" -
