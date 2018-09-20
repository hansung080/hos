#!/bin/bash
qemu-system-x86_64 -L . -m 64 -fda "/Users/hansung/Documents/work/ws/os/hans-os/hans-os.img" -hda "/Users/hansung/Documents/work/ws/os/hans-os/hdd.img" -boot a -localtime -M pc -serial tcp:127.0.0.1:7984,server,nowait -smp 2
