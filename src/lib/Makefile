# lib Makefile

OUT_DIR=../../build/output/lib
SRC_DIR_FROM_OUT=../../../src/lib

NASM64=nasm -f elf64
GCC64=x86_64-pc-linux-gcc -m64 -ffreestanding -mcmodel=large -fno-common
AR=x86_64-pc-linux-ar rcs

TARGET=$(OUT_DIR)/libhos.a

# Build Rules
all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

dep:
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile internal-dep

$(TARGET): dep
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile libhos.a
	@echo $(TARGET) has been created successfully.

install:

clean:
	rm -rf $(OUT_DIR)

# This part below is called by 'make' command above and processed in OUT_DIR.
C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.c)
C_OBJS=$(patsubst %.c, %.o, $(notdir $(C_SRCS)))
ASM_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.asm)
ASM_OBJS=$(patsubst %.asm, %.o, $(notdir $(ASM_SRCS)))

internal-dep:
	$(GCC64) -MM $(C_SRCS) > dependency.dep

%.o: $(SRC_DIR_FROM_OUT)/%.c
	$(GCC64) -c -o $@ $<

%.o: $(SRC_DIR_FROM_OUT)/%.asm
	$(NASM64) -o $@ $<

libhos.a: $(C_OBJS) $(ASM_OBJS)
	$(AR) $@ $^

ifeq (dependency.dep, $(wildcard dependency.dep))
include dependency.dep
endif
