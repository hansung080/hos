# test-elf Makefile

OUT_DIR=../../../build/output/apps/test_elf
SRC_DIR_FROM_OUT=../../../../src/apps/test_elf

GCC64=x86_64-pc-linux-gcc -m64 -ffreestanding -mcmodel=large -fno-common
LD64=x86_64-pc-linux-ld -melf_x86_64 -T $(SRC_DIR_FROM_OUT)/../linker_scripts/elf_x86_64.x -nostdlib -e _start -Ttext 0x0000
OBJCOPY=x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss
READELF=x86_64-pc-linux-readelf
OBJDUMP=x86_64-pc-linux-objdump -M intel

TARGET=$(OUT_DIR)/test-elf.elf

# Build Rules
all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

dep: 
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile internal-dep

internal-build: dep
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile output.elf

$(TARGET): internal-build
	$(OBJCOPY) $(OUT_DIR)/output.elf $@
	@echo $(TARGET) has been created successfully.

install:
	cp -f $(TARGET) ../../tools/network_transfer
	cp -f $(TARGET) $(OUT_DIR)/../../tools/network_transfer

clean:
	rm -rf $(OUT_DIR)
	rm -f ../../tools/network_transfer/event-monitor.elf
	rm -f $(OUT_DIR)/../../tools/network_transfer/event-monitor.elf

readelf: all
	$(READELF) -a $(TARGET)

objdump: all
	$(OBJDUMP) -d $(TARGET)

# This part below is called by 'make' command above and processed in OUT_DIR.
C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/*.c)
C_OBJS=$(patsubst %.c, %.o, $(notdir $(C_SRCS)))

internal-dep:
	$(GCC64) -MM $(C_SRCS) > dependency.dep

%.o: $(SRC_DIR_FROM_OUT)/%.c
	$(GCC64) -c -o $@ $<

output.elf: $(C_OBJS)
	$(LD64) -r -o $@ $^

ifeq (dependency.dep, $(wildcard dependency.dep))
include dependency.dep
endif