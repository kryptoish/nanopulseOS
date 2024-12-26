#!/bin/sh
set -e
. ./iso.sh

qemu-system-$(./target-triplet-to-arch.sh $HOST) -cdrom nanopulseos.iso


# For xlaunch:

# export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0
