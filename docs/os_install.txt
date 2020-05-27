###### Setting up OS Development Environment ######

### download binutils, gcc and install libraries (mac) ###
$ curl https://ftp.gnu.org/gnu/binutils/binutils-2.31.tar.gz -o binutils-2.31.tar.gz
$ curl https://ftp.gnu.org/gnu/gcc/gcc-8.2.0/gcc-8.2.0.tar.gz -o gcc-8.2.0.tar.gz
$ brew install gmp
$ brew install mpfr
$ brew install mpc
$ brew install libmpc

### install cygwin (windows) ###
# create directories
C:\work\cygwin64
C:\work\cygwin64\packages

# download web page
https://cygwin.com/install.html

# execute the downloaded file
setup-x86_64.exe

# Select Root Install Directory
-> C:\work\cygwin64

# Select Local Package Directory
-> C:\work\cygwin64\packages

# Select Your Internet Connection
-> Direct Connection

# Choose A Download Site
-> ftp://ftp.kaist.ac.kr
-> ftp://ftp.jaist.ac.jp

# Select Packages - Devel
- binutils   : bin, src
- bison      : bin
- flex       : bin
- gcc-core   : bin, src
- gcc-g++    : bin
- libtool    : bin
- make       : bin
- patchutils : bin
- texinfo    : bin

# Select Packages - Libs
- libgmp-devel     : bin
- libiconv         : bin
- libintl-devel    : bin
- libmpc-devel     : bin
- libmpfr-devel    : bin
- libncurses-devel : bin
- libopenssl100    : bin, auto checked
- libreadline7     : bin, auto checked

# set Path
Path -> ;C:\work\cygwin64\bin;C:\work\cygwin64\usr\cross\bin

# edit '.bashrc' file (execute cygwin terminal as admin)
$ vi .bashrc
--------------------------------------------------
### config by hs.kwon ###

alias ll='ls -alF'
alias work='cd /home/hansu/work'

export PS1='[`whoami`@cygwin]:`pwd`$ '

echo '.bashrc'
--------------------------------------------------

# check if gcc supports both 32-bit and 64-bit
# (It's the test result on 64-bit windows)
$ vi hello.c
--------------------------------------------------
#include <stdio.h>

int main(int argc, const char** argv) {
	printf("Hello, world!\n");
	return 0
}
--------------------------------------------------
$ gcc -m32 -o hello32 hello.c -> failure: 'hello32' file has not been created.
$ gcc -m64 -o hello64 hello.c -> success: 'hello64' file has been created and executed successfully.

### build binutils: create cross linker ###
# execute the following commands on cygwin terminal
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
--------------------------------------------------
modify <Makefile> before make install
  347 #MAKEINFO = /usr/src/binutils-2.29-1.src/binutils-gdb/missing makeinfo
  348 MAKEINFO = makeinfo
--------------------------------------------------
$ make install

# check result
$ /usr/cross/bin/x86_64-pc-linux-ld --help | grep "supported targets" -> elf64-x86-64 elf32-i386
$ /usr/cross/bin/x86_64-pc-linux-ld --help | grep "supported emulations" -> elf_x86_64 elf32_x86_64 elf_i386

### build gcc: create cross compiler ###
# execute the following commands on cygwin terminal
$ cd /usr/src/gcc-7.3.0-3.src
$ tar xvfJ gcc-7.3.0.tar.xz
skip patch
# patch -p1 < gcc-7.3.0-3.src.patch
# patch -p1 < gcc-7.3.0-3.cygwin.patch
$ cd /usr/src/gcc-7.3.0-3.src/gcc-7.3.0
$ export TARGET=x86_64-pc-linux
$ export PREFIX=/usr/cross
$ export PATH=$PATH:$PREFIX/bin
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib
# If gmp.h or mpfr.h or mpc.h is not found, execute 'configure' command with 'with' options.
$ ./configure --target=$TARGET --prefix=$PREFIX --disable-nls --enable-languages=c --without-headers --disable-shared --enable-multilib --with-gmp=/usr/local/include --with-mpfr=/usr/local/include --with-mpc=/usr/local/include
$ ls -alF config.status -> exist
$ ls -alF Makefile -> exist
$ make configure-host
$ cp /lib/gcc/x86_64-pc-cygwin/7.3.0/libgcc_s.dll.a /lib/gcc/x86_64-pc-cygwin/7.3.0/libgcc_s.a
$ cp /lib/libmpfr.dll.a /lib/libmpfr.a
$ cp /lib/libgmp.dll.a /lib/libgmp.a
$ cp /lib/libmpc.dll.a /lib/libmpc.a
$ make all-gcc
$ make install-gcc

# check result
$ /usr/cross/bin/x86_64-pc-linux-gcc -dumpspecs | grep -A1 multilib_options -> m64/m32

### install nasm (mac) ###
$ brew install nasm

### install nasm (windows) ###
# download web page
https://www.nasm.us/

# execute nasm installer as admin
nasm-2.13.03-installer-x64.exe

# copy nasm to bin directory
- file: nasm.exe
- from: C:\Users\hansu\AppData\Local\bin\NASM
- to: C:\work\cygwin64\bin

# check version
$ nasm -version -> 2.13.03

### install eclipse (optional) ###
# install jdk from download web page
http://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html

# set Path
Path -> ;C:\Program Files\Java\jdk1.8.0_111\bin

# check version
java -version -> 1.8.0_111

# install eclipse from download web page
http://www.eclipse.org/downloads/

# set Properties
eclipse > Properties > C/C++ General > Paths and Symbols > includes (Languages: GNU C)
-> C:/work/cygwin64/usr/include
-> C:/work/cygwin64/lib/gcc/x86_64-pc-cygwin/5.4.0/include

### install qemu (mac) ###
$ brew install qemu

### install qemu (windows) ###
# install qemu from zip file
qemu-0.10.4.zip

# back up original file and modify copied file.
- original file: qemu-x86_64.bat_org
- copied file: qemu-x86_64.bat
- modify: qemu-system-x86_64.exe -L . -m 64 -fda "C:/work/ws/os/hos/hos.img" -hda "C:/work/ws/os/hos/hdd.img" -boot a -localtime -M pc -serial tcp:127.0.0.1:7984,server,nowait -smp 16

# create hdd image
> qemu-img.exe create hdd.img 20M
> chmod 644 hdd.img

# move hdd image
- file: hdd.img
- from: C:\work\qemu
- to: C:\work\ws\os\hos
