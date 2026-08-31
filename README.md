<h1 align="center">
  <img src="res/kernie-logo.svg" alt="Kernie" width="520">
</h1>

A small kernel written from scratch, targeting x86-64, arm64 and riscv64, with no external bootloader and no C library. Everything above the architecture layer is shared, including the shell, the input queue and the freestanding helpers.

On x86-64 a 512-byte BIOS boot sector brings the CPU from real mode into long mode, loads the kernel from disk, and hands control to C code driving the VGA text buffer, the serial port and a PS/2 keyboard. On arm64 and riscv64 QEMU loads `kernel.elf` on the `virt` machine and jumps straight to it, with a UART as the only console and input device, a PL011 on arm64 and an NS16550A on riscv64.

## Features

Shared by both targets:

* Line-buffered interactive shell with backspace support
* Serial console output, with input queued in a ring buffer and drained from the idle loop
* Freestanding `memcpy`, `memmove`, `memset` and `memcmp`
* A cleared `.bss` and a console and architecture layer that keeps `kernel/` portable

x86-64 only:

* 512-byte MBR bootloader that enters 64-bit long mode
* Identity-mapped paging for the first 4MB using 2MB pages
* Interrupt handling with a full IDT, remapped PIC, and stubs for exceptions 0-31 and IRQs 0-15
* CPU exception reporting to both VGA and serial, with the faulting vector and RIP
* VGA 80x25 text output with scrolling
* Interrupt-driven serial input on IRQ 4, so the kernel is usable over `-nographic`
* PS/2 keyboard driver translating scan code set 1 to ASCII, with shift support

arm64 only:

* Boots as an ELF on the QEMU `virt` machine, parking secondary cores
* PL011 UART console, with input polled from the idle loop since there is no GIC yet
* Reboot and power off through PSCI

riscv64 only:

* Boots in machine mode with no firmware beneath it, parking secondary harts
* NS16550A UART console, with input polled from the idle loop since there is no trap vector yet
* Reboot and power off through the SiFive test device

## Requirements

* `nasm` to assemble the x86-64 boot sector and ISR stubs
* A cross toolchain for the target, `x86_64-elf-gcc`, `aarch64-elf-gcc` or `riscv64-elf-gcc`
* `qemu-system-x86_64`, `qemu-system-aarch64` or `qemu-system-riscv64` to run the result

The host compiler only works when it targets the right architecture and object format, so a cross toolchain is required on macOS, where the system `gcc` is Apple clang targeting arm64 Mach-O. `build.sh` picks up `x86_64-elf-gcc` and `x86_64-elf-ld` automatically whenever they are installed, and both can be overridden with the `CC` and `LD` environment variables.

```bash
# macOS
brew install nasm qemu x86_64-elf-gcc x86_64-elf-binutils
brew install aarch64-elf-gcc aarch64-elf-binutils   # for the arm64 target
brew install riscv64-elf-gcc riscv64-elf-binutils   # for the riscv64 target

# Debian and Ubuntu
sudo apt install nasm qemu-system-x86 qemu-system-arm qemu-system-misc build-essential
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu       # arm64
sudo apt install gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu       # riscv64
```

## Building

`ARCH` selects the target and defaults to `x86_64`.

```bash
./build.sh                # x86-64, produces os.bin
ARCH=arm64 ./build.sh     # arm64, produces kernel.elf
ARCH=riscv64 ./build.sh   # riscv64, produces kernel.elf
```

The x86-64 build assembles the boot sector, assembles the ISR stubs, compiles each `.c` file freestanding, links a flat binary at `0x100000`, concatenates the boot sector and the kernel into `os.bin`, and pads the image to 32KB. It fails rather than producing a broken image if the kernel outgrows either the 30 sectors the boot sector reads (15360 bytes) or the 32KB image pad.

The arm64 and riscv64 builds have no boot sector, since QEMU loads `kernel.elf` directly and jumps to `_start`.

The arm64 build is compiled with `-mstrict-align`, because its MMU is not enabled yet, so memory is treated as Device type and an unaligned access raises an alignment fault. The riscv64 build instead uses `-march=rv64imac`, an ISA string without the floating point extensions, so the compiler cannot reach a register file that is never enabled, together with `-mcmodel=medany` because the kernel is linked at `0x80000000`.

Object files land in `build/$ARCH/`.

## Running

```bash
qemu-system-x86_64 -drive format=raw,file=os.bin
```

Or without a window, driving the kernel entirely over the serial port:

```bash
qemu-system-x86_64 -drive format=raw,file=os.bin -nographic
```

The arm64 and riscv64 builds are always serial only, since the `virt` machine has no text buffer:

```bash
qemu-system-aarch64 -machine virt -cpu cortex-a72 -kernel kernel.elf -nographic
qemu-system-riscv64 -machine virt -bios none -kernel kernel.elf -nographic
```

The riscv64 target runs with `-bios none`, so the kernel owns machine mode with no SBI firmware beneath it.

On boot the shell prints a colorized ASCII splash and a prompt. The version and
architecture are defined in `kernel/version.h`; the build date and time are embedded by
the compiler on every build.

```text
  --------------------------------------------------------------------------

        ||      //////      _  __ _____ ____  _   _ ___ _____
        ||    //////        | |/ /| ____|  _ \| \ | |_ _| ____|
        ||<<                | ' / |  _| | |_) |  \| || ||  _|
        ||    \\\\\\        | . \ | |___|  _ <| |\  || || |___
        ||      \\\\\\      |_|\_\|_____|_| \_\_| \_|___|_____|

               A tiny kernel with unreasonable ambitions.

  --------------------------------------------------------------------------
      VERSION  0.1.0-dev    ARCH  x86_64    BUILT  Aug 30 2026 10:28:30
             [ CONSOLE OK ]  [ SERIAL OK ]  [ SHELL READY ]
  --------------------------------------------------------------------------

  Type 'help' for available commands.

>
```

## Shell commands

| Command      | Description                 |
| ------------ | --------------------------- |
| `help`       | show the available commands |
| `clear`      | clear the screen            |
| `echo <msg>` | print a message             |
| `tick`       | show the timer tick count   |
| `reboot`     | reboot the system           |
| `shutdown`   | power off the machine       |

## Memory map

| Address      | Contents                                     |
| ------------ | -------------------------------------------- |
| `0x00001000` | page tables, PML4 then PDPT then PD          |
| `0x00007C00` | boot sector, as loaded by the BIOS           |
| `0x00008000` | staging buffer for the kernel read from disk |
| `0x00090000` | stack top, grows down                        |
| `0x000B8000` | VGA text buffer                              |
| `0x00100000` | kernel, copied here after entering long mode |

## Layout

Sources are grouped by what they depend on, so that portable code stays free of architecture specifics as the kernel grows. Everything under `kernel/` and `lib/` builds unchanged for both targets.

| Directory | Contents |
| --- | --- |
| `arch/x86_64/` | x86-64 boot, interrupts and console |
| `arch/arm64/` | arm64 boot, console and architecture hooks |
| `arch/riscv64/` | riscv64 boot, console and architecture hooks |
| `kernel/` | architecture neutral core and the shell |
| `drivers/` | hardware drivers |
| `lib/` | freestanding helpers shared across the kernel |
| `res/` | branding assets |

| File | Description |
| --- | --- |
| `arch/x86_64/boot.asm` | boot sector, real mode to long mode, loads the kernel |
| `arch/x86_64/isr.asm` | interrupt stubs, save registers, call C, `iretq` |
| `arch/x86_64/idt.c`, `idt.h` | IDT gate descriptors, PIC remapping, end of interrupt |
| `arch/x86_64/interrupts.c`, `interrupts.h` | IDT setup and the C interrupt dispatcher |
| `arch/x86_64/io.h` | `inb`, `outb` and `outw` port helpers |
| `arch/x86_64/console.c` | console over both the VGA text buffer and the UART |
| `arch/x86_64/arch.c` | idle, reboot, shutdown and the tick counter |
| `arch/x86_64/kernel.ld` | linker script placing the flat binary at `0x100000` |
| `arch/arm64/boot.S` | entry point, parks secondary cores and sets the stack |
| `arch/arm64/console.c` | console over the UART alone, colours are discarded |
| `arch/arm64/arch.c` | idle with polled input, plus the unimplemented hooks |
| `arch/arm64/kernel.ld` | linker script placing the ELF at `0x40080000` |
| `arch/riscv64/boot.S` | entry point, parks secondary harts and sets the stack |
| `arch/riscv64/console.c` | console over the UART alone, colours are discarded |
| `arch/riscv64/arch.c` | idle with polled input, plus reboot and power off |
| `arch/riscv64/kernel.ld` | linker script placing the ELF at `0x80000000` |
| `kernel/kernel.c` | `kernel_main`, brings up the subsystems and idles |
| `kernel/console.h` | console interface and colour attributes |
| `kernel/arch.h` | the hooks each architecture has to provide |
| `kernel/shell.c`, `shell.h` | line-buffered command shell |
| `kernel/input.c`, `input.h` | ring buffer sink for keyboard and serial input |
| `kernel/version.h` | version, architecture and build stamp |
| `kernel/elf.h` | ELF64 structures, not wired up yet |
| `drivers/vga.c`, `vga.h` | VGA 80x25 text output, x86-64 only |
| `drivers/serial.c`, `serial.h` | UART formatting shared by every port |
| `drivers/uart_16550.c` | 16550 UART behind x86-64 port I/O |
| `drivers/uart_pl011.c` | PL011 UART on the arm64 virt machine |
| `drivers/uart_ns16550.c` | NS16550A UART on the riscv64 virt machine |
| `drivers/keyboard.c`, `keyboard.h` | PS/2 scan code set 1 to ASCII, x86-64 only |
| `lib/mem.c`, `mem.h` | freestanding `memcpy`, `memmove`, `memset` and `memcmp` |
| `build.sh` | build script, `ARCH` selects the target |
