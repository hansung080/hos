# utils Makefile

OUT_DIR=../../build/output/utils

all: prepare image-maker network-transfer

prepare:
	mkdir -p $(OUT_DIR)

image-maker:
	@echo
	@echo image-maker build start.
	
	make -C image_maker
	
	@echo image-maker build end.

network-transfer:
	@echo
	@echo network-transfer build start.
	
	make -C network_transfer
	
	@echo network-transfer build end.
	@echo

install:
	make -C image_maker install
	make -C network_transfer install

clean:
	make -C image_maker clean
	make -C network_transfer clean
	rm -rf $(OUT_DIR)
