# apps Makefile

OUT_DIR=../../build/output/apps

all: prepare event-monitor test-elf text-viewer

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

text-viewer:
	@echo
	@echo text-viewer build start.
	
	make -C text_viewer
	
	@echo text-viewer build end.

install:
	make -C event_monitor install
	make -C test_elf install
	make -C text_viewer install

clean:
	make -C event_monitor clean
	make -C test_elf clean
	make -C text_viewer clean
	rm -rf $(OUT_DIR)