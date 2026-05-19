BUILD_DIR ?= ../build/kernel
PLATFORM ?= qemu

CC := clang
LD := ld.lld

RISCV64_DIR := arch/riscv64
DEV_DIR := dev
INCLUDE_DIR := include
MACHINE_DIR := machine/$(PLATFORM)
ARCH_INCLUDE_DIR := $(RISCV64_DIR)/include
LIB_DIR := lib
FLANTERM_ROOT := ../third_party/flanterm
FLANTERM_SRC_DIR := $(FLANTERM_ROOT)/src
FLANTERM_BUILD_DIR := $(BUILD_DIR)/third_party/flanterm

RISCV64_FLAGS := \
	-target riscv64-unknown-elf \
	-march=rv64gc \
	-mabi=lp64 \
	-mcmodel=medany

CFLAGS := \
	$(RISCV64_FLAGS) \
	-ffreestanding \
	-fno-builtin \
	-fno-pic \
	-fno-stack-protector \
	-fno-omit-frame-pointer \
	-I $(INCLUDE_DIR) \
	-I $(ARCH_INCLUDE_DIR) \
	-I $(FLANTERM_SRC_DIR) \
	-I $(FLANTERM_SRC_DIR)/flanterm_backends \
	-nostdinc \
	-nostdlib \
	-O2 \
	-Wall \
	-Wextra \
	-Werror

ASFLAGS := \
	$(RISCV64_FLAGS) \
	-ffreestanding \
	-nostdlib

LDFLAGS := \
	-T $(RISCV64_DIR)/linker.ld \
	-nostdlib \
	-static

KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin

OBJS := \
	$(BUILD_DIR)/$(RISCV64_DIR)/boot.o \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/$(LIB_DIR)/string.o \
	$(BUILD_DIR)/$(DEV_DIR)/fw_cfg/fw_cfg.o \
	$(BUILD_DIR)/$(DEV_DIR)/ramfb/ramfb.o \
	$(FLANTERM_BUILD_DIR)/flanterm.o \
	$(FLANTERM_BUILD_DIR)/flanterm_backends/fb.o \
	$(BUILD_DIR)/$(MACHINE_DIR)/uart.o

.PHONY: all bin clean

all: $(KERNEL_ELF)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(FLANTERM_BUILD_DIR)/flanterm.o: $(FLANTERM_SRC_DIR)/flanterm.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(FLANTERM_BUILD_DIR)/flanterm_backends/fb.o: $(FLANTERM_SRC_DIR)/flanterm_backends/fb.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(OBJS) $(RISCV64_DIR)/linker.ld
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

bin: $(KERNEL_ELF)
	@command -v llvm-objcopy >/dev/null 2>&1 || { \
		echo "error: llvm-objcopy is required for 'make bin'"; \
		exit 1; \
	}
	llvm-objcopy -O binary $(KERNEL_ELF) $(KERNEL_BIN)

clean:
	rm -rf $(BUILD_DIR)
