#!/bin/bash
set -euxo pipefail

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

# TODO check the output
"$BUILDDIR/lc3as" < "$SRCDIR/test/all-instructions.asm"
