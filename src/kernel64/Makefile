# kernel64 Makefile

OUT_DIR=../../build/output/kernel64
SRC_DIR_FROM_OUT=../../../src/kernel64

NASM64=nasm -f elf64
GCC64=x86_64-pc-linux-gcc -m64 -ffreestanding
LD64=x86_64-pc-linux-ld -melf_x86_64 -T $(SRC_DIR_FROM_OUT)/linker_scripts/elf_x86_64.x -nostdlib -e k_main -Ttext 0x200000
OBJCOPY64=x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

TARGET=$(OUT_DIR)/kernel64.bin

# Build Rules
all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

dep:
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile internal-dep

internal-build: dep
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile kernel64.elf

$(TARGET): internal-build
	$(OBJCOPY64) $(OUT_DIR)/kernel64.elf $@

install:

clean:
	rm -rf $(OUT_DIR)

# This part below is called by the make command above and processed in OUT_DIR.
EP_SRC=$(SRC_DIR_FROM_OUT)/entry_point.s
EP_OBJ=entry_point.o
C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.c)
ASM_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.asm)
C_OBJS=$(notdir $(patsubst %.c, %.o, $(C_SRCS)))
ASM_OBJS=$(notdir $(patsubst %.asm, %.o, $(ASM_SRCS)))

$(EP_OBJ): $(EP_SRC)
	$(NASM64) -o $@ $<

%.o: $(SRC_DIR_FROM_OUT)/%.c
	$(GCC64) -c $<

%.o: $(SRC_DIR_FROM_OUT)/%.asm
	$(NASM64) -o $@ $<

internal-dep:
	$(GCC64) -MM $(C_SRCS) > dependency.dep
	
kernel64.elf: $(EP_OBJ) $(C_OBJS) $(ASM_OBJS)
	$(LD64) -o $@ $^
 
ifeq (dependency.dep, $(wildcard dependency.dep))
include dependency.dep
endif
