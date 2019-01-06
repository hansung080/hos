# tools Makefile

OUT_DIR=../../build/output/tools

all: prepare dep-modifier image-maker network-transfer bin2c

prepare:
	mkdir -p $(OUT_DIR)

dep-modifier:
	@echo
	@echo dep-modifier build start.
	
	make -C dep_modifier
	
	@echo dep-modifier build end.

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

bin2c:
	@echo
	@echo bin2c build start.
	
	make -C bin2c_
	
	@echo bin2c build end.
	@echo

install:
	make -C image_maker install
	make -C dep_modifier install
	make -C network_transfer install
	make -C bin2c_ install

clean:
	make -C image_maker clean
	make -C dep_modifier clean
	make -C network_transfer clean
	make -C bin2c_ clean
	rm -rf $(OUT_DIR)
