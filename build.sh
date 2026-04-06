#!/bin/bash
set -e

echo "=== Step 1: Assemble bootloader ==="
echo "  Real mode entry point, sets up long mode, loads kernel from disk"
nasm -f bin boot.asm -o boot.bin

echo ""
echo "=== Step 2: Assemble ISR stubs ==="
echo "  Interrupt service routine entry points (save regs, call C, iretq)"
nasm -f elf64 isr.asm -o isr.o

echo ""
echo "=== Step 3: Compile kernel C code ==="
echo "  -ffreestanding       : no stdlib, we ARE the OS"
echo "  -fno-stack-protector : no __stack_chk_fail (doesn't exist here)"
echo "  -mno-red-zone        : unsafe in kernel, interrupts would clobber it"
echo "  -fno-pic             : no position-independent code overhead"
echo "  -c                   : compile only, don't link yet"
CFLAGS="-ffreestanding -fno-stack-protector -mno-red-zone -fno-pic -m64 -c"
gcc $CFLAGS -o kernel.o kernel.c
gcc $CFLAGS -o vga.o vga.c
gcc $CFLAGS -o serial.o serial.c
gcc $CFLAGS -o idt.o idt.c
gcc $CFLAGS -o keyboard.o keyboard.c
gcc $CFLAGS -o interrupts.o interrupts.c

echo ""
echo "=== Step 4: Link kernel into flat binary ==="
echo "  -T kernel.ld    : use our linker script (load at 0x100000)"
echo "  --oformat binary : raw flat binary, no ELF headers"
ld -T kernel.ld -o kernel.bin \
    kernel.o vga.o serial.o idt.o keyboard.o interrupts.o isr.o \
    --oformat binary

echo ""
echo "=== Step 5: Combine boot sector + kernel into disk image ==="
echo "  boot.bin   = first 512 bytes (sector 1)"
echo "  kernel.bin = sectors 2+ (loaded by BIOS int 0x13)"
cat boot.bin kernel.bin > os.bin

echo ""
echo "=== Step 6: Pad to floppy size (optional, helps some emulators) ==="
# pad to at least 32KB
truncate -s 32K os.bin

echo ""
echo "=== Done! ==="
echo "Bootloader: $(stat -c%s boot.bin) bytes"
echo "Kernel:     $(stat -c%s kernel.bin) bytes"
echo "Disk image: $(stat -c%s os.bin) bytes"
echo ""
echo "Run with:"
echo "  qemu-system-x86_64 -drive format=raw,file=os.bin"
echo ""
echo "Or in console mode (no GUI):"
echo "  qemu-system-x86_64 -drive format=raw,file=os.bin -nographic"
