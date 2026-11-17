CROSS_COMPILE ?= aarch64-none-linux-gnu-

CC      := $(CROSS_COMPILE)gcc
OBJCOPY := $(CROSS_COMPILE)objcopy
READELF := $(CROSS_COMPILE)readelf

BUILD := build

CFLAGS := \
    -std=gnu11 \
    -ffixed-x18 \
    -O2 \
    -g \
    -Wall \
    -Wextra \
    -Werror \
    -ffreestanding \
    -fno-builtin \
    -fno-stack-protector \
    -fpie \
    -mgeneral-regs-only \
    -fno-unwind-tables \
    -fno-asynchronous-unwind-tables \
    -Iinclude

LDFLAGS := \
    -nostdlib \
    -pie \
    -Wl,-T,linker.ld \
    -Wl,--build-id=none \
    -Wl,--no-dynamic-linker \
    -Wl,-Bsymbolic

OBJS := \
    $(BUILD)/entry.o \
    $(BUILD)/vectors.o \
    $(BUILD)/ffa_smc.o \
    $(BUILD)/ffa.o \
    $(BUILD)/runtime.o \
    $(BUILD)/main.o

.PHONY: all clean inspect rebuild

all: $(BUILD)/ffa-ref-test-sp.bin

$(BUILD):
	mkdir -p $@

$(BUILD)/entry.o: src/entry.S | $(BUILD)
	$(CC) -g -c $< -o $@

$(BUILD)/vectors.o: src/vectors.S | $(BUILD)
	$(CC) -g -c $< -o $@

$(BUILD)/ffa_smc.o: src/ffa_smc.S | $(BUILD)
	$(CC) -g -c $< -o $@

$(BUILD)/ffa.o: src/ffa.c include/ffa/ffa_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/runtime.o: src/runtime.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/main.o: src/main.c include/ffa/ffa_abi.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/ffa-ref-test-sp.elf: $(OBJS) linker.ld
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

$(BUILD)/ffa-ref-test-sp.bin: $(BUILD)/ffa-ref-test-sp.elf
	$(OBJCOPY) -O binary $< $@

inspect: $(BUILD)/ffa-ref-test-sp.elf
	$(READELF) -h -l -S $<

clean:
	rm -rf $(BUILD)

rebuild: clean
	$(MAKE) all
