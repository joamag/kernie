#!/bin/bash
#
# build.sh
#
# Builds the kernel for one target, selected through ARCH.
#
# The two targets do not produce the same kind of artefact. x86_64 links a flat
# binary with no headers and glues it behind a boot sector, because the sector
# has to jump straight to the first byte. arm64 links an ELF and stops there,
# because QEMU reads the program headers and does the loading itself.
#
# The steps are narrated as they run. It is a teaching kernel, and the flags
# involved are not obvious enough to leave unexplained.

set -e

ARCH="${ARCH:-x86_64}"

COMMON_SRC="kernel/kernel.c kernel/shell.c kernel/input.c drivers/serial.c lib/mem.c"

case "$ARCH" in
x86_64)
    # the system gcc/ld are only usable when they target x86-64 ELF, which is
    # not the case on macOS, so prefer a cross toolchain when one is installed
    if command -v x86_64-elf-gcc > /dev/null && command -v x86_64-elf-ld > /dev/null; then
        CC="${CC:-x86_64-elf-gcc}"
        LD="${LD:-x86_64-elf-ld}"
    else
        CC="${CC:-gcc}"
        LD="${LD:-ld}"
    fi
    ARCH_SRC="arch/x86_64/idt.c arch/x86_64/interrupts.c arch/x86_64/console.c arch/x86_64/arch.c"
    ARCH_SRC="$ARCH_SRC drivers/vga.c drivers/keyboard.c drivers/uart_16550.c"
    ARCHFLAGS="-mno-red-zone -mgeneral-regs-only -fno-pic -m64"
    ;;
arm64)
    # prefer the bare metal toolchain, fall back to the Debian and Ubuntu
    # gcc-aarch64-linux-gnu one, which works fine for freestanding code
    if command -v aarch64-elf-gcc > /dev/null && command -v aarch64-elf-ld > /dev/null; then
        CC="${CC:-aarch64-elf-gcc}"
        LD="${LD:-aarch64-elf-ld}"
    else
        CC="${CC:-aarch64-linux-gnu-gcc}"
        LD="${LD:-aarch64-linux-gnu-ld}"
    fi
    ARCH_SRC="arch/arm64/console.c arch/arm64/arch.c drivers/uart_pl011.c"
    ARCHFLAGS="-mgeneral-regs-only -mstrict-align -fno-pic"
    ;;
riscv64)
    # prefer the bare metal toolchain, fall back to the Debian and Ubuntu
    # gcc-riscv64-linux-gnu one, which works fine for freestanding code
    if command -v riscv64-elf-gcc > /dev/null && command -v riscv64-elf-ld > /dev/null; then
        CC="${CC:-riscv64-elf-gcc}"
        LD="${LD:-riscv64-elf-ld}"
    else
        CC="${CC:-riscv64-linux-gnu-gcc}"
        LD="${LD:-riscv64-linux-gnu-ld}"
    fi
    ARCH_SRC="arch/riscv64/console.c arch/riscv64/arch.c drivers/uart_ns16550.c"
    ARCHFLAGS="-march=rv64imac -mabi=lp64 -mcmodel=medany -fno-pic"
    ;;
*)
    echo "ERROR: unsupported ARCH '$ARCH', expected x86_64, arm64 or riscv64"
    exit 1
    ;;
esac

echo "=== Toolchain ==="
echo "  ARCH: $ARCH"
echo "  CC:   $CC"
echo "  LD:   $LD"
echo ""

OBJDIR="build/$ARCH"
rm -rf "$OBJDIR"
mkdir -p "$OBJDIR"

echo "=== Step 1: Assemble the entry point ==="
if [ "$ARCH" = "x86_64" ]; then
    echo "  Real mode entry point, sets up long mode, loads kernel from disk"
    nasm -f bin arch/x86_64/boot.asm -o boot.bin
    echo ""
    echo "=== Step 2: Assemble ISR stubs ==="
    echo "  Interrupt service routine entry points (save regs, call C, iretq)"
    nasm -f elf64 arch/x86_64/isr.asm -o "$OBJDIR/isr.o"
    ASM_OBJ="$OBJDIR/isr.o"
else
    echo "  QEMU loads the ELF and jumps straight to _start, no real mode dance"
    # ARCHFLAGS has to split into separate arguments
    # shellcheck disable=SC2086
    $CC $ARCHFLAGS -c "arch/$ARCH/boot.S" -o "$OBJDIR/boot.o"
    echo ""
    echo "=== Step 2: Assemble ISR stubs ==="
    echo "  Skipped, the arm64 port has no vector table yet"
    ASM_OBJ="$OBJDIR/boot.o"
fi

echo ""
echo "=== Step 3: Compile kernel C code ==="
echo "  -ffreestanding       : no stdlib, we ARE the OS"
echo "  -fno-stack-protector : no __stack_chk_fail (doesn't exist here)"
if [ "$ARCH" = "riscv64" ]; then
    echo "  -march=rv64imac      : no F or D extension, so no floating point"
    echo "  -mcmodel=medany      : the kernel is linked well above the low 2GB"
else
    echo "  -mgeneral-regs-only  : no SIMD, the FPU is never enabled"
fi
if [ "$ARCH" = "arm64" ]; then
    echo "  -mstrict-align       : the MMU is off, so memory is Device type and"
    echo "                         unaligned accesses raise an alignment fault"
fi
echo "  -I.                  : headers are included by path from the root"
echo "  -c                   : compile only, don't link yet"
CFLAGS="-ffreestanding -fno-stack-protector $ARCHFLAGS -I. -c"

# GCC's loop idiom pass can rewrite the mem.c byte loops into calls to
# themselves, clang has no such pass and rejects the flag outright
if $CC -fno-tree-loop-distribute-patterns -E - < /dev/null > /dev/null 2>&1; then
    echo "  -fno-tree-loop-distribute-patterns : no self calls in mem.c"
    CFLAGS="$CFLAGS -fno-tree-loop-distribute-patterns"
fi

OBJS="$ASM_OBJ"
for src in $COMMON_SRC $ARCH_SRC; do
    obj="$OBJDIR/$(basename "${src%.c}").o"
    # CFLAGS has to split into separate arguments
    # shellcheck disable=SC2086
    $CC $CFLAGS -DKERNIE_ARCH="\"$ARCH\"" -o "$obj" "$src"
    OBJS="$OBJS $obj"
done

echo ""
echo "=== Step 4: Link the kernel ==="
if [ "$ARCH" = "x86_64" ]; then
    echo "  -T arch/x86_64/kernel.ld : linker script, load at 0x100000"
    echo "  --oformat binary : raw flat binary, no ELF headers"
    # OBJS has to split into separate arguments
    # shellcheck disable=SC2086
    $LD -T arch/x86_64/kernel.ld -o kernel.bin $OBJS --oformat binary

    # boot.asm reads 30 sectors (15360 bytes) into 0x8000, anything past that
    # is never loaded and the kernel would jump straight into garbage
    KERNEL_MAX=15360
    KERNEL_SIZE=$(( $(wc -c < kernel.bin) ))
    if [ "$KERNEL_SIZE" -gt "$KERNEL_MAX" ]; then
        echo "  ERROR: kernel.bin is $KERNEL_SIZE bytes, boot.asm only loads $KERNEL_MAX"
        echo "         raise 'mov al, 30' in arch/x86_64/boot.asm to read more sectors"
        exit 1
    fi
else
    echo "  -T arch/$ARCH/kernel.ld : linker script for the target"
    # OBJS has to split into separate arguments
    # shellcheck disable=SC2086
    $LD -T "arch/$ARCH/kernel.ld" -o kernel.elf $OBJS
    KERNEL_SIZE=$(( $(wc -c < kernel.elf) ))
fi

echo ""
echo "=== Step 5: Build the bootable image ==="
if [ "$ARCH" = "x86_64" ]; then
    echo "  boot.bin   = first 512 bytes (sector 1)"
    echo "  kernel.bin = sectors 2+ (loaded by BIOS int 0x13)"
    cat boot.bin kernel.bin > os.bin

    # pad to at least 32KB, truncate sets an exact size so the image must never
    # grow past it or the tail would be silently cut back off
    IMAGE_MAX=32768
    IMAGE_SIZE=$(( $(wc -c < os.bin) ))
    if [ "$IMAGE_SIZE" -gt "$IMAGE_MAX" ]; then
        echo "  ERROR: os.bin is $IMAGE_SIZE bytes, larger than the $IMAGE_MAX pad"
        exit 1
    fi
    truncate -s 32K os.bin
else
    echo "  Skipped, QEMU boots kernel.elf directly"
fi

echo ""
echo "=== Done! ==="
echo "Arch:       $ARCH"
if [ "$ARCH" = "x86_64" ]; then
    echo "Bootloader: $(( $(wc -c < boot.bin) )) bytes"
    echo "Kernel:     $KERNEL_SIZE bytes (max $KERNEL_MAX)"
    echo "Disk image: $(( $(wc -c < os.bin) )) bytes"
    echo ""
    echo "Run with:"
    echo "  qemu-system-x86_64 -drive format=raw,file=os.bin"
    echo ""
    echo "Or in console mode (no GUI):"
    echo "  qemu-system-x86_64 -drive format=raw,file=os.bin -nographic"
else
    echo "Kernel:     $KERNEL_SIZE bytes (kernel.elf)"
    echo ""
    echo "Run with:"
    if [ "$ARCH" = "riscv64" ]; then
        echo "  qemu-system-riscv64 -machine virt -bios none -kernel kernel.elf -nographic"
    else
        echo "  qemu-system-aarch64 -machine virt -cpu cortex-a72 -kernel kernel.elf -nographic"
    fi
fi
