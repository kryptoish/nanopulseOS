#!/bin/sh
set -e
. ./build.sh

mkdir -p isodir
mkdir -p isodir/boot
mkdir -p isodir/boot/grub

cp sysroot/boot/nanopulseos.kernel isodir/boot/nanopulseos.kernel
cat > isodir/boot/grub/grub.cfg << EOF
set timeout=3
set default=0

insmod all_video
set gfxmode=auto

menuentry "nanopulseos" {
	multiboot2 /boot/nanopulseos.kernel
}
EOF
grub-mkrescue --modules="multiboot2 all_video" -o nanopulseos.iso isodir

# Before running this file to build run the following prep:
# export PREFIX="$HOME/opt/cross"
# export TARGET=i686-elf
# export PATH="$PREFIX/bin:$PATH"

# For display:
# export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
