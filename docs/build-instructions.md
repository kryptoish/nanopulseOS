# Build Instructions

Lost? Read https://wiki.osdev.org/GCC_Cross-Compiler. This is just the short 

Note: the cross-compiler will be put next to your src code not inside (../)

# Deps

These need to be adjusted based on the linux distro you're using (untested on other distros).

```sh
sudo xbps-install -S base-devel bison flex gmp-devel mpfr-devel libmpc-devel isl-devel texinfo \
    wget nasm grub xorriso mtools qemu-system-i386
```

# Build the cross compiler

```sh
export PREFIX="$HOME/projects/nanopulseOS/opt/cross" # Adjust this such that ur src code is beside ./opt/cross
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

cd ~/projects/nanopulseOS/src
wget https://ftp.gnu.org/gnu/binutils/binutils-2.47.tar.xz # change these to get the newest version
wget https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz
tar xf binutils-2.47.tar.xz && tar xf gcc-16.2.0.tar.xz

mkdir build-binutils && cd build-binutils
../binutils-2.47/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror # make sure to cahnge all versions for ALL
make -j$(nproc) && make install
cd ..

mkdir build-gcc && cd build-gcc
../gcc-16.2.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers --disable-hosted-libstdcxx
make -j$(nproc) all-gcc all-target-libgcc all-target-libstdc++-v3
make install-gcc install-target-libgcc install-target-libstdc++-v3
```

Newer binutils/gcc versions should work too.

### 3. Put cross-compiler on PATH

```sh
export PATH="$HOME/projects/nanopulseOS/opt/cross/bin:$PATH"
```

Check: `i686-elf-gcc --version`

### 4. Build / ISO / run

From the repo root:

```sh
./build.sh # build kernel into ./sysroot
./iso.sh # build + make nanopulseos.iso (grub-mkrescue)
./clean.sh # clean everything

qemu-system-i386 -cdrom nanopulseos.iso # must have qemu to test (32-bit kernel)
```
