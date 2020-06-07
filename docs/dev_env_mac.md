# hOS Development Environment for Mac
This document explains how to set up hOS development environment for `Mac`.

# Download Toolchains
Download binutils and GCC sources.
You can choose any directories for toolchains.

```sh
$ mkdir -p /usr/local/cross/x86_64-pc-linux
$ cd /usr/local/cross/x86_64-pc-linux
$ curl https://ftp.gnu.org/gnu/binutils/binutils-2.31.tar.gz -o binutils-2.31.tar.gz
$ curl https://ftp.gnu.org/gnu/gcc/gcc-8.2.0/gcc-8.2.0.tar.gz -o gcc-8.2.0.tar.gz
```

Install dependent libraries in order to build toolchains.

```sh
$ brew install gmp
$ brew install mpfr
$ brew install mpc
$ brew install libmpc
```

# Build Toolchains
### Build Binutils
Execute the following commands on terminal to build binutils.

```sh
$ cd /usr/local/cross/x86_64-pc-linux
$ tar xvfz binutils-2.31.tar.gz
$ export TARGET=x86_64-pc-linux
$ export PREFIX=/usr/local/cross/x86_64-pc-linux
$ ./configure --target=$TARGET --prefix=$PREFIX --enable-64-bit-bfd --disable-shared --disable-nls
# Check if 'config.status' and 'Makefile' files exist.
$ ls config.status
$ ls Makefile
$ make configure-host
$ make LDFLAGS="-static"
# Modify Makefile before executing the 'make install' command.
$ vi Makefile
--------------------------------------------------
  347 #MAKEINFO = /usr/src/binutils-2.29-1.src/binutils-gdb/missing makeinfo
  348 MAKEINFO = makeinfo
--------------------------------------------------
$ make install
```

Check if binutils has been built successfully. \
The following letters will be printed in the output if it's been succeeded.

```sh
$ /usr/local/cross/x86_64-pc-linux/bin/x86_64-pc-linux-ld --help | grep "supported targets"
supported targets: elf64-x86-64 elf32-i386

$ /usr/local/cross/x86_64-pc-linux/bin/x86_64-pc-linux-ld --help | grep "supported emulations"
supported emulations: elf_x86_64 elf32_x86_64 elf_i386
```

### Build GCC
Execute the following commands on terminal to build GCC.

```sh
$ cd /usr/local/cross/x86_64-pc-linux
$ tar xvfz gcc-8.2.0.tar.gz
$ export TARGET=x86_64-pc-linux
$ export PREFIX=/usr/local/cross/x86_64-pc-linux
$ export PATH=$PATH:$PREFIX/bin
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib
# If gmp.h or mpfr.h or mpc.h is not found, execute 'configure' command with the 'with' options.
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib --with-gmp=/usr/local/include --with-mpfr=/usr/local/include --with-mpc=/usr/local/include
# Check if 'config.status' and 'Makefile' files exist.  
$ ls config.status
$ ls Makefile
$ make configure-host
$ make all-gcc
$ make install-gcc
```

Check if GCC has been built successfully. \
The following letters will be printed in the output if it's been succeeded.

```sh
$ /usr/local/cross/x86_64-pc-linux/bin/x86_64-pc-linux-gcc -dumpspecs | grep -A1 multilib_options
*multilib_options:
m64/m32
```

### Set Environment Variable
Set `PATH` Environment variable and check the versions of toolchains.

```sh
$ vi ~/.zshrc
--------------------------------------------------
CROSS_HOME=/usr/local/cross
export PATH=$PATH:$CROSS_HOME/x86_64-pc-linux/bin
--------------------------------------------------
$ source ~/.zshrc

$ x86_64-pc-linux-gcc --version
x86_64-pc-linux-gcc (GCC) 8.2.0

$ x86_64-pc-linux-ld --version
GNU ld (GNU Binutils) 2.31
```

### Install Nasm
Install Nasm using brew.

```sh
$ brew install nasm
```

Check the version of Nasm to check if it's been installed successfully.

```sh
$ nasm --version
NASM version 2.14.02 compiled on Sep 28 2019
```

# Install IDE
You can choose any IDEs or editors you want to work on. \
`Visual Studio Code` is one of the recommendable IDEs when writing code in C language.

### Install Visual Studio Code
Download VS Code Installer from [Visual Studio Code Download](https://code.visualstudio.com/download) \
And then, install it. \
After running VS Code, install `C/C++` Extension made by Microsoft which can be searched on Extensions menu.

# Install QEMU
Install QEMU using brew.

```sh
$ brew install qemu
```

Check the version of QEMU to check if it's been installed successfully.

```sh
$ qemu-system-x86_64 --version
QEMU emulator version 5.0.0
```

Create HDD image with 20 MB-sized in your workspace directory as below.

```sh
$ cd /Users/hansung/work/ws/os
$ qemu-img create hdd.img 20M
$ chmod 644 hdd.img
```
