# apps Makefile

OUT_DIR=../../build/output/apps

all: prepare event-monitor test-elf

prepare:
	mkdir -p $(OUT_DIR)

event-monitor:
	@echo
	@echo event-monitor build start.
	
	make -C event_monitor
	
	@echo event-monitor build end.

test-elf:
	@echo
	@echo test-elf build start.
	
	make -C test_elf
	
	@echo test-elf build end.

install:
	make -C event_monitor install
	make -C test_elf install

clean:
	make -C event_monitor clean
	make -C test_elf clean
	rm -rf $(OUT_DIR)