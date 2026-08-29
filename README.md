# Kernie

A small x86-64 kernel written from scratch, with no external bootloader and no C library. A 512-byte BIOS boot sector brings the CPU from real mode into long mode, loads the kernel from disk, and hands control to C code that drives the VGA text buffer, the serial port, the PS/2 keyboard, and an interactive shell.

## Features

* 512-byte MBR bootloader that enters 64-bit long mode
* Identity-mapped paging for the first 4MB using 2MB pages
* Interrupt handling with a full IDT, remapped PIC, and stubs for exceptions 0-31 and IRQs 0-15
* CPU exception reporting to both VGA and serial, with the faulting vector and RIP
* VGA 80x25 text output with scrolling
* Serial COM1 output, plus interrupt-driven serial input so the kernel is usable over `-nographic`
* PS/2 keyboard driver translating scan code set 1 to ASCII, with shift support
* Line-buffered interactive shell with backspace support

## Requirements

* `nasm` to assemble the boot sector and the ISR stubs
* An x86-64 ELF toolchain, `gcc` and `ld`
* `qemu-system-x86_64` to run the result

The host compiler only works when it targets x86-64 ELF, so a cross toolchain is required on macOS, where the system `gcc` is Apple clang targeting arm64 and `ld` produces Mach-O. `build.sh` picks up `x86_64-elf-gcc` and `x86_64-elf-ld` automatically whenever they are installed, and both can be overridden with the `CC` and `LD` environment variables.

```bash
# macOS
brew install nasm qemu x86_64-elf-gcc x86_64-elf-binutils

# Debian and Ubuntu
sudo apt install nasm qemu-system-x86 build-essential
```

## Building

```bash
./build.sh
```

This assembles the boot sector, assembles the ISR stubs, compiles each `.c` file freestanding, links everything into a flat binary at `0x100000`, concatenates the boot sector and the kernel into `os.bin`, and pads the image to 32KB.

The build fails rather than producing a broken image if the kernel outgrows either limit it has to fit in: the 30 sectors the boot sector reads from disk (15360 bytes), and the 32KB image pad.

## Running

```bash
qemu-system-x86_64 -drive format=raw,file=os.bin
```

Or without a window, driving the kernel entirely over the serial port:

```bash
qemu-system-x86_64 -drive format=raw,file=os.bin -nographic
```

On boot the shell prints its banner and a prompt:

```text
=== Kernie the Kernel ===
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

| File                           | Description                                           |
| ------------------------------ | ----------------------------------------------------- |
| `boot.asm`                     | boot sector, real mode to long mode, loads the kernel |
| `kernel.ld`                    | linker script placing the flat binary at `0x100000`   |
| `kernel.c`                     | `kernel_main`, brings up the subsystems and idles     |
| `isr.asm`                      | interrupt stubs, save registers, call C, `iretq`      |
| `idt.c`, `idt.h`               | IDT gate descriptors, PIC remapping, end of interrupt |
| `interrupts.c`, `interrupts.h` | IDT setup and the C interrupt dispatcher              |
| `io.h`                         | `inb` and `outb` port helpers                         |
| `vga.c`, `vga.h`               | VGA 80x25 text output                                 |
| `serial.c`, `serial.h`         | COM1 output                                           |
| `keyboard.c`, `keyboard.h`     | PS/2 scan code set 1 to ASCII translation             |
| `input.c`, `input.h`           | common sink for keyboard and serial input             |
| `shell.c`, `shell.h`           | line-buffered command shell                           |
| `elf.h`                        | ELF64 structures, not wired up yet                    |
| `build.sh`                     | build script                                          |
