export PATH="$HOME/opt/cross/bin:$PATH"
i686-elf-as boot.s -o boot.o
i686-elf-gcc -c kernel.c -o kernel.o -std=gnu99 -ffreestanding -O2 -Wall -Wextra
i686-elf-gcc -T linker.ld -o ./bin/nanopulseos.bin -ffreestanding -O2 -nostdlib boot.o kernel.o -lgcc

# nasm -f elf32 boot.asm -o boot.o
# gcc -m32 -c kernel.c -o kernel.o
# gcc -m32 -c ./src/print.c -o ./src/print.o
# ld -m elf_i386 -T link.ld -o ./bin/kernel.bin boot.o kernel.o ./src/print.o
# qemu-system-x86_64 -kernel ./bin/kernel.bin
