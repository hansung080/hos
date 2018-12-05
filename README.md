HansOS is a 64-bit, multi-core, lightweight, and general OS.
It provides a variety of functions general OS fundamentally have to provide.
The functions HanOS provides and other helpful infomations are written below.

< How to Build >
First of all, clone HansOS git-hub repository to your local computer.
$ git clone https://github.com/hansung080/hans-os.git

To set up OS development environment, refer to the text file below.
-> docs/os_install.txt

Put the commands below on the root directory of HansOS repository to build it,
and check if 'hans-os.img' file has been created successfully.
$ make clean
$ make
$ ls -alF hans-os.img

The created 'hans-os.img' file is a bootable image, so you can boot and run it
on your computer or QEMU.

< HanOS Functions >
- boot-loader: support floppy disk and usb booting.
- memory management: use segmentation and 4 level paging using 2MB-sized page.
- keyboard driver
- PIC driver: process interrupts using interrupt handlers.
- console shell
- timer drivers: consist of PIT, TSC, RTC driver
- multitasking and multithreading
- task scheduling: use Multilevel Queue Scheduler which is the upgraded version of Round Robin Scheduler.
- synchronization: use mutex and spinlock.
- float operation: use hard float operation with FPU.
- dynamic memory management: use Buddy Block Algorithm to prevent external fragmentation.
- hard disk driver
- RAM disk driver
- file system: use cache and provide C standard in/out functions.
- serial port driver
- multi-core processing: process interrupt load balancing and task load balancing.
- GUI system: consist of 2D graphics libraries, mouse driver, window, window manager, event queue
- GUI tasks: prototype, event-monitor, system-menu, app-panel, system-monitor, shell, image-viewer, color-picker
- system call and user libraries
- applications: event-monitor, test-elf, text-viewer

< Shorthands >
- ID: IDentification
- IO: Input Output
- ETC: ETCetera
- ASCII: American Standard Code for Information Interchange
- ACK: ACKnowledge
- MP: MultiProcessor
- SIMD: Single Instruction Multiple Data
- FIFO: Fist In, Fist Out (queue)
- LIFO: Last In, Fist Out (stack)
- BIOS: Basic Input Output System
- CPU: Central Processing Unit
- FPU: Floating Point Unit
- RAM: Random Access Memory
- ROM: Read Only Memory
- HDD: Hard Disk Drive
- RDD: Ram Disk Drive
- PIC: Programmable Interrupt Controller
- APIC: Advanced Programmable Interrupt Controller
- IOAPIC: Input Output Advanced Programmable Interrupt Controller
- PIT: Programmable Interval Timer
- TSC: Time Stamp Counter
- RTC: Real-Time Clock
- PML4T: Page Map Level 4 Table
- PDPT: Page Directory Pointer Table
- PD: Page Directory
- PT: Page Table
- GDT: Global Descriptor Table
- GDTR: Global Descriptor Table Register (register)
- IDT: Interrupt Descriptor Table
- IDTR: Interrupt Descriptor Table Register (register)
- TSS: Task State Segment
- TR: Task Register (register)
- TS: Task-Switched (field of CR0)
- TCB: Task Control Block
- MBR: Master Boot Record
- CHS: Cylinder-Head-Sector
- LBA: Logical Block Addressing
- OEM: Original Equipment Manufacturer
- ISA: Industry Standard Architecture (bus)
- EBDA: Extended Bios Data Area
- IRQ: Interrupt ReQuest
- ISR: Interrupt Service Routine
- IST: Interrupt Stack Table
- NMI: Non-Maskable Interrupt
- INTIN: INTerrupt INput (pin)
- LINTIN: Local INTerrupt INput (pin)
- EOI: End Of Interrupt
- EXB: EXecute disaBle (field of page table entry)
- CPUID: Central Processing Unit IDentification (command)
- BSP: BootStrap Processor
- AP: Application Processor
- LED: Light Emitting Diode
- ECC: Error Correcting Code
- VBE: Vesa Bios Extention
- GUI: Graphic User Interface
- JPEG: Joint Photographic Experts Group
- MSR: Model-Specific Register
- ELF: Executable and Linkable Format

< Abbreviation >
- init: initialize
- alloc: allocate
- calc: calculate
- recv: receive
- str: string
- len: length
- char: character
- attr: attribute
- addr: address
- func: function
- arg: argument
- param: parameter
- pos: position
- mem: memory
- info: information
- dir: directory
- config: configuration
- src: source
- dest: destination
- assign: assignment
- prev: previous
- seq: sequential
- asm: assembly
- util: utility
- sync: synchronization
- dep: dependency
- ret: return
- ptr: pointer
- rect: rectangle
- han: hangul
- eng: english
- over: overlapped
- app: application
- sys: system
- syscall: system call
- rand: random

< Comment >
- [NOTE]: important comment
- [REF]: reference comment
- [TODO]: to do
- [TEMP]: temporary code

< Task Log >
- [task name info]: info message
- [task name debug]: debug message
- [task name error]: error message

< Window Color >
- reference site
https://digitalsynopsis.com/
https://digitalsynopsis.com/design/beautiful-color-schemes-combinations-palettes/
https://webgradients.com/

- blue: RGB(109, 213, 237) #6dd5ed -> graph bar, button clicked
- blue: RGB(33, 147, 176) #2193b0 -> title bar background active
- orange: RGB(255, 184, 140) #ffb88c -> not used
- orange: RGB(222, 98, 98) #de6262 -> mouse over
- gray: RGB(229, 229, 229) #e5e5e5 -> bright shadow 
- gray: RGB(167, 173, 186) #a7adba -> title bar background inactive
- gray: RGB(101, 115, 126) #65737e -> not used
- gray: RGB(86, 86, 86) #565656 -> dark shadow
- peach: RGB(255, 236, 210) #ffecd2 -> system background
- peach: RGB(252, 182, 159) #fcb69f -> system background mark dark
- green: RGB(212, 252, 121) #d4fc79 -> not used
- green: RGB(150, 230, 161) #96e6a1 -> not used
- green: RGB(132, 250, 176) #84fab0 -> not used
- green: RGB(143, 211, 244) #8fd3f4 -> not used
