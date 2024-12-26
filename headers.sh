#!/bin/sh
set -e
. ./config.sh

mkdir -p "$SYSROOT"

for PROJECT in $SYSTEM_HEADER_PROJECTS; do
  (cd $PROJECT && DESTDIR="$SYSROOT" $MAKE install-headers)
done

# For new shell sessions (If can't find i686-elf)

# export PREFIX="$HOME/opt/cross"
# export TARGET=i686-elf
# export PATH="$PREFIX/bin:$PATH"
