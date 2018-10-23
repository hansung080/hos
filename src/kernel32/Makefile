# kernel32 Makefile

SRC_DIR=.
OUT_DIR=../../build/output/kernel32
SRC_DIR_FROM_OUT=../../../src/kernel32

NASM=nasm
NASM32=nasm -f elf32
GCC32=x86_64-pc-linux-gcc -m32 -ffreestanding
LD32=x86_64-pc-linux-ld -melf_i386 -T $(SRC_DIR_FROM_OUT)/linker_scripts/elf_i386.x -nostdlib -e k_main -Ttext 0x10200
OBJCOPY=x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

TARGET=$(OUT_DIR)/kernel32.bin

# Build Rules
all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

$(OUT_DIR)/entry_point.bin: $(SRC_DIR)/entry_point.s
	$(NASM) -o $@ $<

dep:
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile internal-dep

internal-build: dep
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile kernel32.elf

$(OUT_DIR)/kernel32.elf.bin: internal-build
	$(OBJCOPY) $(OUT_DIR)/kernel32.elf $@

$(TARGET): $(OUT_DIR)/entry_point.bin $(OUT_DIR)/kernel32.elf.bin
	cat $^ > $@

install:

clean:
	rm -rf $(OUT_DIR)

# This part below is called by 'make' command above and processed in OUT_DIR.
C_EP_OBJ=main.o
C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.c)
C_OBJS=$(subst $(C_EP_OBJ), , $(patsubst %.c, %.o, $(notdir $(C_SRCS))))
ASM_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.asm)
ASM_OBJS=$(patsubst %.asm, %.o, $(notdir $(ASM_SRCS)))

internal-dep:
	$(GCC32) -MM $(C_SRCS) > dependency.dep

%.o: $(SRC_DIR_FROM_OUT)/%.c
	$(GCC32) -c -o $@ $<

%.o: $(SRC_DIR_FROM_OUT)/%.asm
	$(NASM32) -o $@ $<

kernel32.elf: $(C_EP_OBJ) $(C_OBJS) $(ASM_OBJS)
	$(LD32) -o $@ $^

ifeq (dependency.dep, $(wildcard dependency.dep))
include dependency.dep
endif