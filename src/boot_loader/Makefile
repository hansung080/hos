# boot-loader Makefile

SRC_DIR=.
OUT_DIR=../../build/output/boot_loader

SRC=$(SRC_DIR)/boot_loader.asm

TARGET=$(OUT_DIR)/boot_loader.bin

all: prepare $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

$(TARGET): $(SRC)
	nasm -o $@ $^

install:

clean:
	rm -rf $(OUT_DIR)
