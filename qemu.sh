#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom nanopulseos.iso -no-reboot -d int

# qemu-system-i386 -cdrom nanopulseos.iso -curses

# For xlaunch:

# export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
