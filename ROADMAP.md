Tracking issue for the path from the current interactive shell to a kernel that can load ELF binaries and talk to the internet.

## Where we are

As of `be95f26` the kernel boots from its own 512 byte MBR into long mode, identity maps the first 4MB, and runs an interactive shell over VGA and serial. It has an IDT with stubs for exceptions 0-31 and IRQs 0-15, a remapped PIC, a PS/2 keyboard driver, interrupt driven serial input, freestanding `mem` functions, and a cleared `.bss`. Input is queued in a ring buffer and drained from the idle loop, so the shell never runs behind a closed interrupt gate.

The kernel is 7664 bytes.

## The constraint that gates everything

`boot.asm` reads 30 sectors, so the kernel cannot exceed **15360 bytes**. We are at 7664, leaving roughly 7.7KB. A TCP/IP stack, an ELF loader and a disk driver together are comfortably over 100KB, so nothing further down this list fits until this is solved.

There is a second, related awkwardness: the kernel cannot load ELF and is itself not ELF, so it has no symbols, no sections and no debugger support.

## Phase 0: lift the size ceiling

Two viable routes, listed with a recommendation rather than as alternatives to debate forever.

- [ ] Decide between a two stage bootloader and adopting Limine (see Open decisions)
- [ ] Build the kernel as an ELF binary rather than a flat image
- [ ] Move to a higher half virtual address
- [ ] Obtain a real memory map instead of assuming 4MB
- [ ] Confirm the kernel boots and the shell still works end to end

## Phase 1: memory management

Everything is statically allocated today. Nothing in either end goal is reachable without this.

- [ ] Parse the memory map into a usable region list
- [ ] Physical frame allocator, bitmap over usable RAM
- [ ] Virtual memory manager, real 4 level page table map and unmap at arbitrary addresses
- [ ] Kernel heap, `kmalloc` and `kfree`
- [ ] Guard pages or at least a panic on allocator exhaustion

## Phase 2: debugging and safety groundwork

Cheap, unglamorous, and it makes every later phase tractable.

- [ ] `kprintf` with `%d`, `%u`, `%s`, `%x`, `%p`. Only hex printing exists today, which will not survive debugging a TCP state machine
- [ ] `panic()` and `assert()`
- [ ] Register dump on unhandled exceptions, not just the vector name and RIP
- [ ] TSS with an IST stack for `#DF`, so a stack overflow produces a diagnosable panic instead of a triple fault and silent reboot

## Phase 3: time

Networking is full of timeouts, so this is a networking prerequisite rather than a nicety.

- [ ] Program the PIT to a known frequency. `tick_count` currently accumulates at the default 18.2Hz because the PIT is never initialised
- [ ] `sleep_ms` and a monotonic uptime counter
- [ ] Consider the APIC timer as a later replacement

## Phase 4: testing

- [x] Commit the QEMU harness as `test.sh`, covering toolchain selection, both builds and the shell on each target
- [ ] GitHub Actions workflow running `build.sh` on ubuntu-latest
- [ ] Host compiled unit tests for logic that does not need a kernel, such as checksums, the ARP table and the TCP state machine

## Phase 5: ELF loading

`elf.h` already carries the ELF64 structures and a `load_elf` skeleton, but nothing includes it.

- [ ] Wire `load_elf` up against a boot module, so the loader can be proven before there is any disk or filesystem
- [ ] Validate `e_machine`, `e_type` and `p_align`, and bounds check `p_vaddr` and `p_memsz` before copying. The current skeleton would write anywhere the header asks it to
- [ ] Map `PT_LOAD` segments through the virtual memory manager rather than assuming they are already mapped
- [ ] Honour `p_flags` for page permissions
- [ ] Decide whether loaded binaries run in kernel space or ring 3 (see Open decisions)

If ring 3 is in scope:

- [ ] User code and data segments in the GDT
- [ ] TSS with a kernel stack for privilege transitions
- [ ] Syscall entry via `SYSCALL` and `SYSRET`
- [ ] A minimal syscall surface, enough for write and exit

Programs written for kernie need no C library and can be cross compiled freestanding today, which is enough to prove the loader. Running anything written by somebody else needs a libc, and that only makes sense once ring 3 and a syscall path exist.

- [ ] Port picolibc, which targets embedded systems and needs a smaller stub surface than newlib
- [ ] Implement the stubs it expects, at minimum `write`, `read`, `close`, `lseek`, `fstat`, `isatty`, `sbrk` and `exit`
- [ ] Provide a `crt0` that sets up the stack, clears bss and calls `main`
- [ ] Back `sbrk` with the kernel heap so `malloc` works in user programs
- [ ] Build an unmodified third party program against it and run it
- [ ] Optionally build binutils and GCC against a custom target triple, so `x86_64-kernie-gcc` knows the ABI, the libc and the default linker script

## Phase 6: storage and filesystem

Needed only to load ELF binaries from disk rather than from a boot module.

- [ ] Block device driver. `virtio-blk` is considerably simpler than AHCI under QEMU, and ATA PIO is simpler still if speed does not matter
- [ ] Read only tar initrd, which is roughly a hundred lines and enough to hold real binaries
- [ ] FAT32 later, once the image needs to be written to or edited from the host

## Phase 7: networking

Largely independent of Phase 5 once Phase 1 is done, so the two can progress in parallel.

- [ ] PCI enumeration through configuration space at `0xCF8` and `0xCFC`, including BAR decoding
- [ ] NIC driver (see Open decisions)
- [ ] Ethernet framing, send and receive
- [ ] ARP with a cache and expiry
- [ ] IPv4 with fragmentation handling
- [ ] ICMP echo reply
- [ ] UDP
- [ ] DHCP client
- [ ] TCP, starting with the handshake and moving to retransmission and windowing
- [ ] DNS resolver
- [ ] A trivial HTTP client

## Milestones

Each of these is externally observable, which makes them worth aiming at directly.

| Milestone | Proves |
| --- | --- |
| Kernel boots past 15KB | Phase 0 |
| `kmalloc` survives a stress loop | Phase 1 |
| Stack overflow prints a panic instead of rebooting | Phase 2 |
| An ELF boot module runs and returns | Phase 5 |
| A program built against picolibc runs unmodified | The libc port |
| `arping` from the host gets a reply | Ethernet and ARP |
| **The kernel answers `ping`** | IPv4 and ICMP |
| The kernel obtains a DHCP lease | UDP and DHCP |
| A TCP handshake completes against a host listener | TCP |
| `GET /` returns real content from a real server | The end goal |

## Open decisions

**Bootloader.** A two stage loader keeps everything in tree and teaches more, but it will be rewritten later and does not solve the ELF or memory map problems. Limine hands over a loaded ELF kernel, a memory map, a higher half mapping and boot modules, and boot modules in particular let Phase 5 start before Phase 6 exists. Recommendation is Limine.

**NIC.** `rtl8139` is the simplest well documented card and QEMU models it faithfully, which makes it the best first driver. `e1000` is more realistic and has better documentation but real descriptor rings. `virtio-net` is quickest to working but only exists under virtualisation. Recommendation is `rtl8139` first, with `e1000` as a second driver once the stack above it is stable.

**Ring 3.** Loading ELF binaries into kernel space is much less work and is enough to prove the loader. Real user mode brings the GDT, TSS and syscall work listed in Phase 5. Worth deciding explicitly rather than drifting into it.

**C library.** picolibc is built for embedded targets, has a smaller required stub surface and a simpler build than newlib, which is the traditional hobby kernel choice but heavier. Either way the libc is the easy half; the syscalls underneath it are the work, and the port is gated on ring 3 existing first. Recommendation is picolibc.

## Notes on testing networking

QEMU user mode networking with `-netdev user` provides DHCP, DNS and NAT with no host configuration, so the internet goal is reachable without touching host network settings. Adding `-object filter-dump` captures a pcap that opens directly in Wireshark, which turns stack debugging from guesswork into inspection.

## Deliberately out of scope for now

Multiprocessing, SMP, preemptive scheduling, a graphical console, USB, and audio. None of them are on the path to either goal.
