# hans-os Makefile

TARGET_DEP=build/output/boot_loader/boot_loader.bin \
           build/output/kernel32/kernel32.bin \
           build/output/kernel64/kernel64.bin

TARGET=hans-os.img

all: prepare tools boot-loader kernel32 kernel64 $(TARGET) lib apps

prepare:
	mkdir -p build

tools:
	@echo
	@echo tools build start.
	
	make -C src/tools
	
	@echo tools build end.

boot-loader:
	@echo
	@echo boot-loader build start.
	
	make -C src/boot_loader
	
	@echo boot-loader build end.

kernel32:
	@echo
	@echo kernel32 build start.
	
	make -C src/kernel32
	
	@echo kernel32 build end.

kernel64:
	@echo
	@echo kernel64 build start.
	
	make -C src/kernel64
	
	@echo kernel64 build end.

$(TARGET): $(TARGET_DEP)
	@echo
	@echo image-maker start.
	
	build/output/tools/image_maker/image-maker $@ $^
	chmod 644 $@
	
	@echo image-maker end.
	@echo $(TARGET) has been created successfully.

lib:
	@echo
	@echo lib build start.
	
	make -C src/lib
	
	@echo lib build end.

apps:
	@echo
	@echo apps build start.
	
	make -C src/apps
	
	@echo apps build end.
	@echo
	@echo all builds complete.

install:
	make -C src/tools install
	make -C src/boot_loader install
	make -C src/kernel32 install
	make -C src/kernel64 install
	make -C src/lib install
	make -C src/apps install

clean:
	make -C src/tools clean
	make -C src/boot_loader clean
	make -C src/kernel32 clean
	make -C src/kernel64 clean
	make -C src/lib clean
	make -C src/apps clean
	rm -rf build
	rm -f $(TARGET)
