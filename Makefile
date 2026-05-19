KERNEL_DIR := kernel
BUILD_DIR := build
KERNEL_BUILD_DIR := $(BUILD_DIR)/kernel
PLATFORM ?= qemu

QEMU := qemu-system-riscv64

KERNEL_ELF := $(KERNEL_BUILD_DIR)/kernel.elf
KERNEL_BIN := $(KERNEL_BUILD_DIR)/kernel.bin

.PHONY: all kernel bin run clean

all: kernel

kernel:
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=../$(KERNEL_BUILD_DIR) PLATFORM=$(PLATFORM)

submodules:
	git submodule update --init --recursive

bin: kernel
	$(MAKE) -C $(KERNEL_DIR) BUILD_DIR=../$(KERNEL_BUILD_DIR) PLATFORM=$(PLATFORM) bin

run: kernel
	@command -v $(QEMU) >/dev/null 2>&1 || { \
		echo "error: $(QEMU) is not installed"; \
		echo "install QEMU and rerun 'make run'"; \
		exit 1; \
	}
	@printf '%s\n' "running QEMU with display backend: $(QEMU_DISPLAY)"
	$(QEMU) \
		-machine virt \
		-m 128M \
		-serial stdio \
		-bios none \
		-device ramfb \
		-kernel $(KERNEL_ELF)

clean:
	rm -rf $(BUILD_DIR)