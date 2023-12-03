#!/bin/bash
set -euxo pipefail

# if unset we'll expect our input to reside in the directory alongside our script
DIR=$(dirname "$0")
SRCDIR=${SRCDIR:-$DIR/..}
BUILDDIR=${BUILDDIR:-$DIR/..}

result=$("$BUILDDIR/lc3vm" -i "$SRCDIR/test/hello-world.obj" -o-)

if [ "$result" != "hello world!" ] ; then
    exit 1
fi
