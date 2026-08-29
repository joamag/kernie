#!/bin/bash
set -e

# the system gcc/ld are only usable when they target x86-64 ELF, which is not
# the case on macOS, so prefer a cross toolchain whenever one is installed
if command -v x86_64-elf-gcc > /dev/null && command -v x86_64-elf-ld > /dev/null; then
    CC="${CC:-x86_64-elf-gcc}"
    LD="${LD:-x86_64-elf-ld}"
else
    CC="${CC:-gcc}"
    LD="${LD:-ld}"
fi

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
echo "  -mgeneral-regs-only  : no SSE/x87, CR4.OSFXSR is never set so they #UD"
echo "  -fno-pic             : no position-independent code overhead"
echo "  -c                   : compile only, don't link yet"
CFLAGS="-ffreestanding -fno-stack-protector -mno-red-zone -mgeneral-regs-only -fno-pic -m64 -c"

# GCC's loop idiom pass can rewrite the mem.c byte loops into calls to
# themselves, clang has no such pass and rejects the flag outright
if $CC -fno-tree-loop-distribute-patterns -E - < /dev/null > /dev/null 2>&1; then
    echo "  -fno-tree-loop-distribute-patterns : no self calls in mem.c"
    CFLAGS="$CFLAGS -fno-tree-loop-distribute-patterns"
fi
$CC $CFLAGS -o kernel.o kernel.c
$CC $CFLAGS -o vga.o vga.c
$CC $CFLAGS -o serial.o serial.c
$CC $CFLAGS -o idt.o idt.c
$CC $CFLAGS -o input.o input.c
$CC $CFLAGS -o keyboard.o keyboard.c
$CC $CFLAGS -o shell.o shell.c
$CC $CFLAGS -o interrupts.o interrupts.c
$CC $CFLAGS -o mem.o mem.c

echo ""
echo "=== Step 4: Link kernel into flat binary ==="
echo "  -T kernel.ld    : use our linker script (load at 0x100000)"
echo "  --oformat binary : raw flat binary, no ELF headers"
$LD -T kernel.ld -o kernel.bin \
    kernel.o vga.o serial.o idt.o input.o keyboard.o shell.o interrupts.o mem.o isr.o \
    --oformat binary

# boot.asm reads 30 sectors (15360 bytes) into 0x8000, anything past that is
# never loaded and the kernel would jump straight into garbage
KERNEL_MAX=15360
KERNEL_SIZE=$(( $(wc -c < kernel.bin) ))
if [ "$KERNEL_SIZE" -gt "$KERNEL_MAX" ]; then
    echo "  ERROR: kernel.bin is $KERNEL_SIZE bytes, boot.asm only loads $KERNEL_MAX"
    echo "         raise 'mov al, 30' in boot.asm to read more sectors"
    exit 1
fi

echo ""
echo "=== Step 5: Combine boot sector + kernel into disk image ==="
echo "  boot.bin   = first 512 bytes (sector 1)"
echo "  kernel.bin = sectors 2+ (loaded by BIOS int 0x13)"
cat boot.bin kernel.bin > os.bin

echo ""
echo "=== Step 6: Pad to floppy size (optional, helps some emulators) ==="
# pad to at least 32KB, truncate sets an exact size so the image must never
# grow past it or the tail would be silently cut back off
IMAGE_MAX=32768
IMAGE_SIZE=$(( $(wc -c < os.bin) ))
if [ "$IMAGE_SIZE" -gt "$IMAGE_MAX" ]; then
    echo "  ERROR: os.bin is $IMAGE_SIZE bytes, larger than the $IMAGE_MAX pad"
    exit 1
fi
truncate -s 32K os.bin

echo ""
echo "=== Done! ==="
echo "Bootloader: $(( $(wc -c < boot.bin) )) bytes"
echo "Kernel:     $KERNEL_SIZE bytes (max $KERNEL_MAX)"
echo "Disk image: $(( $(wc -c < os.bin) )) bytes"
echo ""
echo "Run with:"
echo "  qemu-system-x86_64 -drive format=raw,file=os.bin"
echo ""
echo "Or in console mode (no GUI):"
echo "  qemu-system-x86_64 -drive format=raw,file=os.bin -nographic"
