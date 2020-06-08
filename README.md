# hOS
hOS is a 64-bit, multi-core, and lightweight operating system. hOS provides fundamental features of operating system, such as memory management, task scheduling, file system, GUI system, etc. Someone who wants to understand the inside of operating system and make one of his own will get a help by looking into the inside of hOS.

# Features
* Boot-loader - implements floppy disk and USB booting.
* Memory Management - implements segmentation and 4-level paging with 2MB-sized page.
* Keyboard Driver
* PIC Driver - handles interrupts using ISR.
* Console Shell
* Timer Drivers - consists of PIT, TSC, RTC driver.
* Multitasking / Multithreading
* Task Scheduling - implements Multilevel Queue Scheduler which is the upgrade version of Round Robin Scheduler.
* Synchronization - implements Mutex and Spinlock.
* Floating-point Operation - implements the hardware floating-point operation with FPU.
* Dynamic Memory Management - implements Buddy Block Algorithm to prevent external fragmentation.
* Hard Disk Driver
* RAM Disk Driver
* File System - implements cache and provides the interface similar to C standard I/O functions.
* Serial Port Driver
* Multi-core Processing - implements the interrupt load balancing and task load balancing.
* GUI System - implements 2D graphics libraries, mouse driver, window, window manager, and event queue.
* Widgets - see below for the details.
* System Applications - see below for the details.
* System Call / User Libraries
* User Applications - see below for the details.

# Setup Development Environment
Before you build hOS, you must set up hOS development environment to build toolchains and install QEMU which provides the virtual environment to run hOS.
You can see the appropriate document below depending on your host PC.
- [hOS Development Environment for Windows]
- [hOS Development Environment for Mac] 

# Build and Run hOS
First, clone hOS repository to your local PC.

```sh
$ git clone https://github.com/hansung080/hos.git
```

And then, build hOS.

```sh
$ cd hos
$ make

# check if hOS image has been created successfully
$ ls hos.img
```

And then, run hOS on QEME.

```
$ ./scripts/run_qemu-x86_64.sh
```

[hOS](https://github.com/hansung080/hos/blob/master/images/hos.png)

# Applications
### Widgets
- Menu
- Logo
- Button
- Clock

### System Applications
- Prototype
- Event Monitor
- System Menu
- App Panel
- System Monitor
- Shell
- Image Viewer
- Color Picker
- Alert
- Confirm

### User Applcations
- Event Monitor
- Test Elf
- Text Viewer

[hOS Development Environment for Windows]: <https://github.com/hansung080/hos/blob/master/docs/dev_env_win.md>
[hOS Development Environment for Mac]: <https://github.com/hansung080/hos/blob/master/docs/dev_env_mac.md>
