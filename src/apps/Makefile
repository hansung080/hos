# apps Makefile

OUT_DIR=../../build/output/apps

all: prepare event-monitor

prepare:
	mkdir -p $(OUT_DIR)

event-monitor:
	@echo
	@echo event-monitor build start.
	
	make -C event_monitor
	
	@echo event-monitor build end.

install:
	make -C event_monitor install

clean:
	make -C event_monitor clean
	rm -rf $(OUT_DIR)