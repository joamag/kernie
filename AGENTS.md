# AGENTS.md

## Building

The build system is a single shell script. `ARCH` selects the target and defaults to `x86_64`:

```bash
./build.sh                # x86-64, produces os.bin
ARCH=arm64 ./build.sh     # arm64, produces kernel.elf
```

The host compiler is only usable when it already targets the right architecture and object format, so a cross toolchain is required on macOS, where the system `gcc` is Apple clang targeting arm64 Mach-O. The script prints the toolchain it selected before anything else, which is the first thing to read when a build behaves unexpectedly:

```text
=== Toolchain ===
  ARCH: arm64
  CC:   aarch64-linux-gnu-gcc
  LD:   aarch64-linux-gnu-ld
```

Selection prefers the bare metal `*-elf-*` tools and falls back to the Debian and Ubuntu `aarch64-linux-gnu-*` ones, which are fine because nothing links against a libc. Both `CC` and `LD` override either path.

```bash
# macOS
brew install nasm qemu x86_64-elf-gcc x86_64-elf-binutils
brew install aarch64-elf-gcc aarch64-elf-binutils

# Debian and Ubuntu
sudo apt install nasm qemu-system-x86 qemu-system-arm build-essential
sudo apt install gcc-aarch64-linux-gnu binutils-aarch64-linux-gnu
```

Object files land in `build/$ARCH/`, so switching targets never reuses stale objects.

## Running

```bash
qemu-system-x86_64 -drive format=raw,file=os.bin -nographic
qemu-system-aarch64 -machine virt -cpu cortex-a72 -kernel kernel.elf -nographic
```

The arm64 target is serial only, the `virt` machine has no text buffer and no PS/2 controller.

## Testing

```bash
./test.sh
```

The harness builds every target and boots each one under QEMU, driving the shell over the serial port and checking what comes back. It covers toolchain selection, `CC` and `LD` overrides, `ARCH` validation, both builds, the shell commands, a pasted burst, reboot, power off and that secondary arm64 cores stay parked.

Input reaches the guest through a fifo rather than a pipe, so that `$!` is QEMU itself. A pipeline reports the job instead, and `wait` then blocks on the feeder rather than the emulator, which silently turns "did the guest exit" into "did my own sleep finish". Any new guest test should go through `run_guest` rather than open its own pipeline.

QEMU is much slower without hardware acceleration, so the waits are tunable and the integration raises them:

```bash
BOOT_WAIT=8 CMD_WAIT=2 ./test.sh
```

There is no unit test framework and no coverage tooling. Coverage instrumentation needs a runtime that writes profile data to a filesystem, which a freestanding kernel does not have, so behaviour is asserted by booting the thing rather than by measuring lines. Host compiled unit tests are planned for the logic that will not need a kernel, such as checksums and the TCP state machine.

## Linting

The shell scripts are the only tooling in the tree and are checked with shellcheck:

```bash
shellcheck build.sh test.sh
```

There is no `.clang-format` and no formatter for the C sources, so match the surrounding file by hand. Where a variable is deliberately left unquoted so that it splits into separate arguments, such as `CFLAGS` or `OBJS`, keep the `# shellcheck disable=SC2086` directive and the comment that explains it.

## Continuous Integration

`.github/workflows/main.yml` lints the scripts, builds both targets on Linux and macOS, builds x86-64 across several GCC versions, and runs `test.sh` under QEMU. `.github/workflows/deploy.yml` publishes bootable images on tags.

The Linux arm64 job is the one that exercises the Debian toolchain fallback, since `aarch64-elf-gcc` is not packaged there. Keep that job, it covers a path no macOS machine can reach.

## Architecture Layer

This is the part of the tree most easily broken by a well meaning change.

`kernel/` and `lib/` compile unchanged for every target and must stay that way. There is not a single `#ifdef` in the tree and there should not be one. Selection happens at link time, through the source list `build.sh` assembles per architecture, so an architecture is added by writing files rather than by branching inside shared ones.

Shared code reaches the machine through two contracts:

- `kernel/console.h` carries `console_init`, `console_clear`, `console_print` and `console_print_hex`, plus the colour attributes. x86-64 mirrors output to the VGA text buffer and the UART, arm64 writes to the UART alone and discards the colour.
- `kernel/arch.h` carries `arch_init`, `arch_idle`, `arch_reboot`, `arch_shutdown` and `arch_ticks`.

`arch_idle` is what the idle loop calls after draining the input queue. On x86-64 it halts until the next interrupt. On arm64 there is no GIC yet, so it polls the UART and feeds `input_handle_char`, which is why `arch_ticks` returns zero there and `tick` always reads zero.

Where things belong:

| Directory | Rule |
| --- | --- |
| `arch/<target>/` | anything that knows the instruction set, the interrupt model or the memory map, including the linker script |
| `kernel/` | portable core, the shell and the input queue, no port I/O and no inline assembly |
| `drivers/` | hardware drivers, selected per architecture by the source list |
| `lib/` | freestanding helpers, portable by definition |

`arch/x86_64/interrupts.c` is architecture code rather than kernel core, because it knows x86 vector numbers, the PIC and end of interrupt. A second architecture would not reuse a line of it.

Adding an architecture means a boot entry point, a linker script, a `console.c`, an `arch.c` implementing the five hooks, a UART driver, and a case in `build.sh`. Nothing above the layer should need to change.

## Freestanding Constraints

These have each cost real debugging time. Read them before changing build flags.

**The kernel has a hard size ceiling on x86-64.** `arch/x86_64/boot.asm` reads 30 sectors, so `kernel.bin` cannot exceed 15360 bytes and the image cannot exceed the 32KB pad. The build fails rather than producing something that jumps into garbage. Raising the ceiling means raising `mov al, 30` in the boot sector.

**GCC emits calls to `memcpy`, `memset`, `memmove` and `memcmp` even under `-ffreestanding`**, for struct assignments and any copy it decides not to inline. `lib/mem.c` provides them. It is also built with `-fno-tree-loop-distribute-patterns`, without which GCC rewrites the byte loop inside `memcpy` into a call to `memcpy` and recurses forever. That flag is feature tested because clang rejects it.

**No SIMD anywhere.** `boot.asm` never sets CR4.OSFXSR, so any SSE instruction the compiler chose to emit would raise an undefined instruction fault. `-mgeneral-regs-only` keeps it out.

**The arm64 MMU is off**, so every access is treated as Device memory, where an unaligned access raises an alignment fault. `VBAR_EL1` is never set, so such a fault hangs the CPU with no output at all. `-mstrict-align` avoids the accesses. Enabling the MMU with a normal memory mapping is the real fix and belongs with the memory management work.

**`.bss` is not part of the loaded image.** `kernel_main` clears the range between `__bss_start` and `__bss_end` before anything else runs, and it must stay the first thing it does.

**There is no `printf`.** Output goes through `console_print` and `console_print_hex`, which prints a fixed width hexadecimal value. A formatted printer is planned and will make debugging anything numeric far less painful.

## Style Guide

- C sources use 4-space indentation, no tabs.
- Braces follow K&R style, the opening brace on the same line as the statement.
- A space before the parenthesis in control structures: `if (condition)`, `for (...)`, `while (...)`.
- Pointer declarations use right alignment: `const char *str`.
- C comments use block style only (`/* ... */`), lowercase, with no trailing period. Assembly uses `;` in NASM sources and `//` in GAS sources.
- Comments explain why rather than what, and the tree leans towards commenting the surprising rather than the obvious.
- Naming uses `snake_case` for functions and variables, `UPPER_CASE` for macros and constants, and typedef struct names are PascalCase without a suffix, such as `IdtEntry` and `InterruptFrame`.
- Header guards are the bare uppercase file name, such as `CONSOLE_H`.
- Includes are path qualified from the tree root, such as `#include "drivers/vga.h"`, and the build passes `-I.`.
- Headers live beside their sources rather than in a separate `include/` tree.
- There is no hard line length limit, though most lines sit well under 100 characters.
- Sources use LF line endings and carry no license header.
- Never use the em dash character, in sources, in documentation or in commit messages.
- Never add AI or assistant attribution to commits, pull requests or issues.

## Commit Messages

This project follows [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/):

```text
<type>: <description>

<body>
```

### Commit Types

| Type       | Description                                             |
| ---------- | ------------------------------------------------------- |
| `feat`     | A new feature or functionality                          |
| `fix`      | A bug fix                                               |
| `docs`     | Documentation only changes                              |
| `refactor` | Code change that neither fixes a bug nor adds a feature |
| `chore`    | Maintenance tasks, dependency updates, build changes    |
| `test`     | Adding or updating tests                                |
| `version`  | Version bump commits, reserved for releases             |

### Guidelines

- Use lowercase for the type prefix, with an optional scope such as `fix(build):`.
- Use imperative mood in the description, "add feature" rather than "added feature".
- Keep the subject line under 80 characters.
- Add an empty line between the subject and the body.
- Make the body a markdown bullet list, dash prefixed, and keep it to three to five lines.
- Reference issue or pull request numbers with `(#123)` where it applies.
- Never add a `Co-Authored-By` trailer.

### Examples

```text
feat: add an arm64 target alongside x86-64
fix: set OUT2 in the serial MCR so COM1 can raise IRQ 4
refactor: organise sources into arch, kernel, drivers and lib
test: add a QEMU harness covering both targets
chore: add GitHub Actions workflows for both targets
version: 0.2.0
```

## Pre-Commit Checklist

- [ ] `shellcheck build.sh test.sh` is clean
- [ ] Both targets build: `./build.sh` and `ARCH=arm64 ./build.sh`
- [ ] `./test.sh` passes
- [ ] `kernel/` and `lib/` still compile for both targets, with no `#ifdef` introduced
- [ ] The kernel is still inside the 15360 byte ceiling on x86-64
- [ ] README and ROADMAP reflect anything that changed about the layout or the targets
- [ ] No debugging output and no commented out code left behind
- [ ] No em dash, and no AI attribution anywhere

## New Release

- Make sure both targets build and `./test.sh` passes.
- Update `KERNIE_VERSION` in `kernel/version.h`. The architecture string is derived from the compiler and the build stamp is filled in automatically, so neither needs touching.
- Create a commit with the message `version: $VERSION_NUMBER`.
- Push the commit and create a tag with the value of the new version number.
- The deploy workflow builds both targets on that tag and attaches `kernie-x86_64.bin` and `kernie-arm64.elf` to a new GitHub release.

There is no `CHANGELOG.md` yet. The plan of record is issue #5 and `ROADMAP.md`, which should be kept in step as phases complete.

## License

There is no `LICENSE` file in this repository yet, so the terms are not currently stated. Do not add license headers to sources until one is chosen.
