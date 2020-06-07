# hOS Development Environment for Windows
This documents explains how to set up hOS developments environments for `Windows`.

# Install Cygwin
### Create Directories
First of all, you need to create two directories in where you want, just like below.
One is for Cygwin, and another is for Cygwin packages.
> C:\work\cygwin64 (Change path if you want) \
> C:\work\cygwin64\packages (Change path if you want)

### Download Cygwin
You can download Cygwin from [Cygwin Install](https://cygwin.com/install.html)
And then, execute the exe file you've just downloaded.
* File Name - setup-x86_64.exe

Select the options just like below while installing Cygwin.
#### 1. Select Root Install Directory
- C:\work\cygwin64

#### 2. Select Local Package Directory
- C:\work\cygwin64\packages

#### 3. Select Your Internet Connection
- Direct Connection

#### 4. Choose A Download Site
You can choose the closest mirror site from where you are.
These are two options if you live in Korea. If these two sites doesn't exist, you can choose another one.
- ftp://ftp.kaist.ac.kr
- ftp://ftp.jaist.ac.jp

#### 5. Select Packages
| Devel | Check |
| ----- | ----- |
| binutils | bin / src |
| bison | bin |
| flex | bin |
| gcc-core | bin / src |
| gcc-g++ | bin |
| libtool | bin |
| make | bin |
| patchutils | bin |
| texinfo | bin |

| Libs | Check |
| ---- | ----- |
| libgmp-devel | bin |
| libiconv | bin |
| libintl-devel | bin |
| libmpc-devel | bin |
| libmpfr-devel | bin |
| libncurses-devel | bin |
| libopenssl100 | bin (already checked) |
| libreadline7  | bin (already checked) |

### Set Environment Variable
Set `Path` environment variable.
One is for cygwin64 binaries, another is for toolchains you will build in the next step.
| Env | Value |
| --- | ----- |
| Path | C:\work\cygwin64\bin;C:\work\cygwin64\usr\cross\bin |

### Execute Cygwin Terminal
You need to execute Cygwin terminal as `admin` in order to get a write permission.
Write `.bashrc` file to customize several settings of linux shell.

```sh
$ vi .bashrc
--------------------------------------------------
# .bashrc

export PS1='`whoami`@cygwin:`pwd`$ '

alias ll='ls -alF'
alias work='cd /home/hansu/work'

echo '.bashrc'
--------------------------------------------------

$ source .bashrc
```

And then, build and run a C program to check if GCC supports both 32-bit and 64-bit.
NOTE: This test was done on `64-bit` Windows.

```sh
$ vi hello.c
--------------------------------------------------
#include <stdio.h>

int main(int argc, const char** argv) {
	printf("Hello, world!\n");
	return 0;
}
--------------------------------------------------

# failure: 'hello32' file has not been created.
$ gcc -m32 -o hello32 hello.c

# success: 'hello64' file has been created and executed successfully.
$ gcc -m64 -o hello64 hello.c
```

# Build Toolchains
### Build Binutils
Execute the following commands on Cygwin terminal to build binutils.

```sh
$ mkdir -p /usr/cross
$ cd /usr/src/binutils-2.29-1.src
$ tar xvf binutils-gdb-2.29.tar.bz2
$ cd /usr/src/binutils-2.29-1.src/binutils-gdb
$ export TARGET=x86_64-pc-linux
$ export PREFIX=/usr/cross
$ ./configure --target=$TARGET --prefix=$PREFIX --enable-64-bit-bfd --disable-shared --disable-nls
$ ls -alF config.status -> exist
$ ls -alF Makefile -> exist
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

Check if binutils has been built successfully.
The following letters will be printed in the output if it's been succeeded.

```sh
$ /usr/cross/bin/x86_64-pc-linux-ld --help | grep "supported targets"
  elf64-x86-64 elf32-i386

$ /usr/cross/bin/x86_64-pc-linux-ld --help | grep "supported emulations"
  elf_x86_64 elf32_x86_64 elf_i386
```

### Build GCC - GNU Compiler Collection
Execute the following commands on Cygwin terminal to build GCC.

```sh
$ cd /usr/src/gcc-7.3.0-3.src
$ tar xvfJ gcc-7.3.0.tar.xz
# You can skip to execute the `patch` command.
# patch -p1 < gcc-7.3.0-3.src.patch
# patch -p1 < gcc-7.3.0-3.cygwin.patch
$ cd /usr/src/gcc-7.3.0-3.src/gcc-7.3.0
$ export TARGET=x86_64-pc-linux
$ export PREFIX=/usr/cross
$ export PATH=$PATH:$PREFIX/bin
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib
# If gmp.h or mpfr.h or mpc.h is not found, execute 'configure' command with the 'with' options.
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib --with-gmp=/usr/local/include --with-mpfr=/usr/local/include --with-mpc=/usr/local/include
# Check if 'config.status' and 'Makefile' files exist.  
$ ls -alF config.status
$ ls -alF Makefile
$ make configure-host
$ cp /lib/gcc/x86_64-pc-cygwin/7.3.0/libgcc_s.dll.a /lib/gcc/x86_64-pc-cygwin/7.3.0/libgcc_s.a
$ cp /lib/libmpfr.dll.a /lib/libmpfr.a
$ cp /lib/libgmp.dll.a /lib/libgmp.a
$ cp /lib/libmpc.dll.a /lib/libmpc.a
$ make all-gcc
$ make install-gcc
```

Check if GCC has been built successfully.
The following letters will be printed in the output if it's been succeeded.

```sh
$ /usr/cross/bin/x86_64-pc-linux-gcc -dumpspecs | grep -A1 multilib_options
  m64/m32
```

### Install Nasm - Netwide Assembler
Download Nasm installer from [Nasm](https://www.nasm.us/).
And then, execute Nasm installer as admin.
* File Name -  nasm-2.13.03-installer-x64.exe

Copy nasm to bin directory.
> File Name - nasm.exe
> Source Directory - C:\Users\hansu\AppData\Local\bin\NASM
> Destination Directory -  C:\work\cygwin64\bin

Check the version of Nasm to check if it's been installed successfully.

```sh
$ nasm -version
  2.13.03
```

# Install IDE - Integrated Development Environment
You can choose any IDE or editor you want to work on.
`Visual Studio Code` or `Eclipse` is one of recommendable IDEs when writing code in C language.

### Install Visual Studio Code
Download VS Code Installer from [Visual Studio Code Download](https://code.visualstudio.com/download)
And then, install it.
After running VS Code, install `C/C++` Extension made by Microsoft which can be searched on Extensions menu.

### Install Eclipse for C/C++ Developers
#### 1. Install JDK - Java Development Kit
Download JDK from [JDK download](http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html).
And then, install it.
Set `Path` environment variable.
| Env | Value |
| --- | ----- |
| Path | C:\Program Files\Java\jdk1.8.0_111\bin |

Check the verison of java to check if it's been installed successfully.

```sh
> java -version
  1.8.0_111 
```

#### 2. Install Eclipse for C/C++ Developers
Download Eclipe for C/C++ Developers from [Eclipse Download](http://www.eclipse.org/downloads/).
And then, install it.
After running Elipse, add include path as below.
> Properties > C/C++ General > Paths and Symbols > includes (Languages: GNU C)
> Add Path - C:/work/cygwin64/usr/include
> Add Path - C:/work/cygwin64/lib/gcc/x86_64-pc-cygwin/5.4.0/include

# Install QEMU
Install QEMU from zip file.
* File Name - qemu-0.10.4.zip

Modify `qemu-x86_64.bat` file as below after copying a backup of original file.
-fda and -hda path must be your workspace path.
> File Name - qemu-x86_64.bat
> Modify Content - qemu-system-x86_64.exe -L . -m 64 -fda "C:/work/ws/os/hos/hos.img" -hda "C:/work/ws/os/hos/hdd.img" -boot a -localtime -M pc -serial tcp:127.0.0.1:7984,server,nowait -smp 16

Create HDD image with 20 MB-sized as below.

```sh
> qemu-img.exe create hdd.img 20M
> chmod 644 hdd.img
```

And the, Move it to your workspace directory.
> File Name - hdd.img
> Source Directory - C:\work\qemu
> Destination Directory - C:\work\ws\os\hos
