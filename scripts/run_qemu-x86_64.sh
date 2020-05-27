#!/bin/zsh
#qemu-system-x86_64 -L . -m 64 -fda "/Users/hansung/work/ws/os/hos/hos.img" -hda "/Users/hansung/work/ws/os/hos/hdd.img" -boot a -localtime -M pc -serial tcp:127.0.0.1:7984,server,nowait -smp 2
qemu-system-x86_64 -L . -m 64 -fda "/Users/hansung/work/ws/os/hos/hos.img" -hda "/Users/hansung/work/ws/os/hos/hdd.img" -boot a -M pc -serial tcp:127.0.0.1:7984,server,nowait -smp 2
