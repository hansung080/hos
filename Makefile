# hans-os Makefile

TARGET_DEP=build/output/boot_loader/boot_loader.bin \
           build/output/kernel32/kernel32.bin \
           build/output/kernel64/kernel64.bin

TARGET=hans-os.img

all: prepare utils boot-loader kernel32 kernel64 $(TARGET)

prepare:
	mkdir -p build

utils:
	@echo
	@echo utils build start.
	
	make -C src/utils
	
	@echo utils build end.

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
	
	build/output/utils/image_maker/image-maker $@ $^
	chmod 644 $@
	
	@echo image-maker end.
	@echo 
	@echo $(TARGET) has been created successfully.
	@echo all builds complete.

install:
	make -C src/utils install
	make -C src/boot_loader install
	make -C src/kernel32 install
	make -C src/kernel64 install

clean:
	make -C src/utils clean
	make -C src/boot_loader clean
	make -C src/kernel32 clean
	make -C src/kernel64 clean
	rm -rf build
	rm -f $(TARGET)
