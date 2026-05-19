# risky
Small RISC-V 64-bit microkernel experiment

## Kernel layout

The kernel is split into a few early microkernel-style boundaries:

- `kernel/arch/riscv64/`: RISC-V 64-bit bootstrap and linker script
- `kernel/main.c`: kernel entry point
- `kernel/dev/`: built-in kernel device code
- `kernel/lib/`: small freestanding runtime helpers
- `kernel/include/`: grouped kernel headers
- `kernel/machine/qemu/`: QEMU machine-specific devices, currently UART

Right now the framebuffer path is split into:

- `kernel/dev/fw_cfg/`: QEMU firmware configuration transport
- `kernel/dev/ramfb/`: QEMU RAMFB setup
- `kernel/main.c`: owns the boot framebuffer descriptor and Flanterm setup

The framebuffer shape used by RAMFB lives in `kernel/include/dev/ramfb.h`, and
`main.c` uses the device interfaces directly instead of going through a separate
kernel API layer.

## Usage

Build the kernel ELF:

```sh
make submodules
make
```

The default platform is QEMU. Override it with:

```sh
make PLATFORM=qemu
```

Run it in QEMU:

```sh
make run
```

`make run` enables QEMU RAMFB with `-device ramfb`, so QEMU should open a graphical display for the framebuffer while serial output is routed to the terminal.

The default QEMU display backend is `sdl`, because some host setups fail to
initialize QEMU's `gtk` backend even when GUI output is otherwise available.
If `sdl` does not suit your setup, override it explicitly:

```sh
make QEMU_DISPLAY=gtk run
```

For a fully headless run:

```sh
make QEMU_DISPLAY=none run
```

Optional flat binary export:

```sh
make bin
```

Build just the kernel from inside `kernel/`:

```sh
make -C kernel
```

## Tooling

- `make` uses `clang` and `ld.lld` targeting `riscv64-unknown-elf`
- `make submodules` initializes the Flanterm git submodule
- `make run` expects `qemu-system-riscv64`
- `make bin` expects `llvm-objcopy`
