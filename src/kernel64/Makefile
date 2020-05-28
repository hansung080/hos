# kernel64 Makefile

OUT_DIR=../../build/output/kernel64
SRC_DIR_FROM_OUT=../../../src/kernel64

NASM64=nasm -f elf64
GCC64=x86_64-pc-linux-gcc -m64 -ffreestanding
LD64=x86_64-pc-linux-ld -melf_x86_64 -T $(SRC_DIR_FROM_OUT)/linker_scripts/elf_x86_64.x -nostdlib -e k_main -Ttext 0x200000
OBJCOPY=x86_64-pc-linux-objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

TARGET=$(OUT_DIR)/kernel64.bin

# Build Rules
all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)
	mkdir -p $(OUT_DIR)/core
	mkdir -p $(OUT_DIR)/utils
	mkdir -p $(OUT_DIR)/fonts
	mkdir -p $(OUT_DIR)/images
	mkdir -p $(OUT_DIR)/gui_tasks
	mkdir -p $(OUT_DIR)/widgets

dep:
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile internal-dep

internal-build: dep
	make -C $(OUT_DIR) -f $(SRC_DIR_FROM_OUT)/Makefile kernel64.elf

$(TARGET): internal-build
	$(OBJCOPY) $(OUT_DIR)/kernel64.elf $@

install:

clean:
	rm -rf $(OUT_DIR)

# This part below is called by 'make' command above and processed in OUT_DIR.
EP_SRC=$(SRC_DIR_FROM_OUT)/core/entry_point.s
EP_OBJ=core/entry_point.o
CORE_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/core/*.c)
CORE_C_OBJS=$(patsubst %.c, core/%.o, $(notdir $(CORE_C_SRCS)))
CORE_ASM_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/core/*.asm)
CORE_ASM_OBJS=$(patsubst %.asm, core/%.o, $(notdir $(CORE_ASM_SRCS)))
UTILS_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/utils/*.c)
UTILS_C_OBJS=$(patsubst %.c, utils/%.o, $(notdir $(UTILS_C_SRCS)))
FONTS_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/fonts/*.c)
FONTS_C_OBJS=$(patsubst %.c, fonts/%.o, $(notdir $(FONTS_C_SRCS)))
IMAGES_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/images/*.c)
IMAGES_C_OBJS=$(patsubst %.c, images/%.o, $(notdir $(IMAGES_C_SRCS)))
GUITASKS_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/gui_tasks/*.c)
GUITASKS_C_OBJS=$(patsubst %.c, gui_tasks/%.o, $(notdir $(GUITASKS_C_SRCS)))
WIDGETS_C_SRCS=$(wildcard $(SRC_DIR_FROM_OUT)/widgets/*.c)
WIDGETS_C_OBJS=$(patsubst %.c, widgets/%.o, $(notdir $(WIDGETS_C_SRCS)))

internal-dep:
	$(GCC64) -MM $(CORE_C_SRCS) > core.dep
	../tools/dep_modifier/dep-modifier core.dep core/
	$(GCC64) -MM $(UTILS_C_SRCS) > utils.dep
	../tools/dep_modifier/dep-modifier utils.dep utils/	
	$(GCC64) -MM $(FONTS_C_SRCS) > fonts.dep
	../tools/dep_modifier/dep-modifier fonts.dep fonts/
	$(GCC64) -MM $(IMAGES_C_SRCS) > images.dep
	../tools/dep_modifier/dep-modifier images.dep images/
	$(GCC64) -MM $(GUITASKS_C_SRCS) > gui_tasks.dep
	../tools/dep_modifier/dep-modifier gui_tasks.dep gui_tasks/
	$(GCC64) -MM $(WIDGETS_C_SRCS) > widgets.dep
	../tools/dep_modifier/dep-modifier widgets.dep widgets/
	cat core.dep utils.dep fonts.dep images.dep gui_tasks.dep widgets.dep > dependency.dep

$(EP_OBJ): $(EP_SRC)
	$(NASM64) -o $@ $<

core/%.o: $(SRC_DIR_FROM_OUT)/core/%.c
	$(GCC64) -c -o $@ $<

core/%.o: $(SRC_DIR_FROM_OUT)/core/%.asm
	$(NASM64) -o $@ $<

utils/%.o: $(SRC_DIR_FROM_OUT)/utils/%.c
	$(GCC64) -c -o $@ $<

fonts/%.o: $(SRC_DIR_FROM_OUT)/fonts/%.c
	$(GCC64) -c -o $@ $<

images/%.o: $(SRC_DIR_FROM_OUT)/images/%.c
	$(GCC64) -c -o $@ $<

gui_tasks/%.o: $(SRC_DIR_FROM_OUT)/gui_tasks/%.c
	$(GCC64) -c -o $@ $<

widgets/%.o: $(SRC_DIR_FROM_OUT)/widgets/%.c
	$(GCC64) -c -o $@ $<

kernel64.elf: $(EP_OBJ) $(CORE_C_OBJS) $(CORE_ASM_OBJS) $(UTILS_C_OBJS) $(FONTS_C_OBJS) $(IMAGES_C_OBJS) $(GUITASKS_C_OBJS) $(WIDGETS_C_OBJS)
	$(LD64) -o $@ $^

ifeq (dependency.dep, $(wildcard dependency.dep))
include dependency.dep
endif