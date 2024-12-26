#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/nanopulseos.kernel isodir/boot/nanopulseos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
menuentry "nanopulseos" {
	multiboot /boot/nanopulseos.kernel
}
EOF
grub-mkrescue -o nanopulseos.iso isodir
