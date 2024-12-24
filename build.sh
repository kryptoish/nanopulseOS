# OPEN XLAUNCH (0 & disable access control) before running then do: bash build.sh

export PATH="$HOME/opt/cross/bin:$PATH"
i686-elf-as boot.s -o boot.o
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o ./bin/nanopulseos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc
cp ./bin/nanopulseos.bin isodir/boot/nanopulseos.bin
rm nanopulseos.iso
grub-mkrescue -o nanopulseos.iso isodir
qemu-system-i386 -cdrom nanopulseos.iso


# NOTE: instructions below of what each thing above does

# These are the commands to create a bootable image (.iso) of the current OS

# mkdir -p isodir/boot/grub
# cp ./bin/nanopulseos.bin isodir/boot/nanopulseos.bin
# cp grub.cfg isodir/boot/grub/grub.cfg
# grub-mkrescue -o nanopulseos.iso isodir


# Use this command to simulate the OS in QEMU

# export DISPLAY=$(cat /etc/resolv.conf | grep nameserver | awk '{print $2}'):0 (to configure display)

# qemu-system-i386 -cdrom myos.iso
# or
# qemu-system-i386 -kernel myos.bin (multiboot kernel booting without bootable medium)


# IMPORTANT NOTE: to rebuild and run new changes do this:

# bash build.sh
# cp ./bin/nanopulseos.bin isodir/boot/nanopulseos.bin
# cp grub.cfg isodir/boot/grub/grub.cfg
# grub-mkrescue -o nanopulseos.iso isodir
# OPEN XLAUNCH (0 & disable access control)
# qemu-system-i386 -cdrom nanopulseos.iso