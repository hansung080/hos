# network-transfer Makefile

SRC_DIR=.
OUT_DIR=../../../build/output/tools/network_transfer

SRC=$(SRC_DIR)/main.c
OBJ=$(OUT_DIR)/main.o

TARGET=$(OUT_DIR)/network-transfer
TARGET_COPY=$(SRC_DIR)/network-transfer
DEPENDENCY=$(OUT_DIR)/dependency.dep

all: prepare dep $(TARGET)

prepare:
	mkdir -p $(OUT_DIR)

dep:
	gcc -MM $(SRC) > $(DEPENDENCY)
	../../../build/output/tools/dep_modifier/dep-modifier $(DEPENDENCY) $(OUT_DIR)/

$(OBJ): $(SRC)
	gcc -c -o $@ $^

$(TARGET): $(OBJ)
	gcc -o $@ $^
	@echo $(TARGET) has been created successfully.

install:
	cp -f $(TARGET) $(TARGET_COPY)

clean:
	rm -rf $(OUT_DIR)
	rm -f $(TARGET_COPY)
	
ifeq ($(DEPENDENCY), $(wildcard $(DEPENDENCY)))
include $(DEPENDENCY)
endif