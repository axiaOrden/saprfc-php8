#!/bin/sh
# Build the test-only mock classic RFCSDK (NOT the SAP RFCSDK).
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

mkdir -p "$here/lib"
${CC:-cc} -std=c99 -O2 -fPIC -Wall -Wextra \
   -I"$here/include" \
   -shared -Wl,-soname,librfccm.so \
   -o "$here/lib/librfccm.so" \
   "$here/src/rfccm_mock.c"

echo "built $here/lib/librfccm.so (test double, not SAP RFCSDK)"
