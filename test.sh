#!/bin/bash
#
# test.sh
#
# Builds every target, boots each one under QEMU and drives the shell over the
# serial port, checking what comes back.
#
# There is no unit test framework here and there cannot easily be coverage
# either, since instrumentation needs a runtime that writes profile data to a
# filesystem and a freestanding kernel has none. Behaviour is therefore
# asserted by booting the thing and typing at it.
#
# The order follows build.sh, toolchain selection first, then the build, then
# what the built kernel actually does.

# qemu is slower without hardware acceleration, so CI raises these
BOOT_WAIT="${BOOT_WAIT:-3}"
CMD_WAIT="${CMD_WAIT:-1}"

PASS=0
FAIL=0

ok () {
    echo "  PASS  $1"
    PASS=$((PASS + 1))
}

no () {
    echo "  FAIL  $1"
    [ -n "$2" ] && echo "        $2"
    FAIL=$((FAIL + 1))
}

# expect_in <description> <needle> <haystack>
expect_in () {
    case "$3" in
        *"$2"*) ok "$1" ;;
        *)      no "$1" "expected to find '$2'" ;;
    esac
}

# expect_not_in <description> <needle> <haystack>
expect_not_in () {
    case "$3" in
        *"$2"*) no "$1" "did not expect '$2'" ;;
        *)      ok "$1" ;;
    esac
}

# stub_dir <name>... - a PATH entry holding executables that name themselves
# and fail, so the build reveals which toolchain it selected
stub_dir () {
    local dir name
    dir=$(mktemp -d)
    for name in "$@"; do
        printf '#!/bin/sh\necho "STUB %s"\nexit 1\n' "$name" > "$dir/$name"
        chmod +x "$dir/$name"
    done
    echo "$dir"
}

# run_guest <arch> <command>... - boots the image and types each command,
# leaves the output in GUEST_OUT and whether qemu survived in GUEST_ALIVE
#
# the input arrives through a fifo rather than a pipe so that $! is qemu
# itself, a pipeline would report the job and wait would block on the feeder
run_guest () {
    local arch="$1" log fifo qpid cmd
    shift
    log=$(mktemp)
    fifo="$(mktemp -u)"
    mkfifo "$fifo"

    if [ "$arch" = "x86_64" ]; then
        qemu-system-x86_64 -drive format=raw,file=os.bin \
            -display none -serial stdio < "$fifo" > "$log" 2>&1 &
    else
        # QEMU_EXTRA has to split into separate arguments
        # shellcheck disable=SC2086
        qemu-system-aarch64 -machine virt -cpu cortex-a72 -m 128 $QEMU_EXTRA \
            -kernel kernel.elf -display none -serial stdio < "$fifo" > "$log" 2>&1 &
    fi
    qpid=$!
    exec 3> "$fifo"

    sleep "$BOOT_WAIT"
    for cmd in "$@"; do
        printf '%s\r' "$cmd" >&3
        sleep "$CMD_WAIT"
    done
    sleep $((CMD_WAIT + 1))

    if kill -0 "$qpid" 2>/dev/null; then
        GUEST_ALIVE=yes
    else
        GUEST_ALIVE=no
    fi
    kill -9 "$qpid" 2>/dev/null
    exec 3>&-
    wait "$qpid" 2>/dev/null
    rm -f "$fifo"

    GUEST_OUT=$(cat "$log")
    rm -f "$log"
}

echo "=== Toolchain selection ==="

out=$(PATH="/usr/bin:/bin" ARCH=arm64 ./build.sh 2>&1)
expect_in "arm64 falls back to the Debian toolchain" "CC:   aarch64-linux-gnu-gcc" "$out"

stubs=$(stub_dir aarch64-elf-gcc aarch64-elf-ld)
out=$(PATH="$stubs:/usr/bin:/bin" ARCH=arm64 ./build.sh 2>&1)
expect_in "arm64 prefers the bare metal toolchain" "CC:   aarch64-elf-gcc" "$out"
expect_not_in "arm64 skips the fallback when elf tools exist" "aarch64-linux-gnu" "$out"
rm -rf "$stubs"

out=$(PATH="/usr/bin:/bin" ARCH=arm64 CC=my-own-cc ./build.sh 2>&1)
expect_in "arm64 honours a CC override" "CC:   my-own-cc" "$out"

# the compiler runs before the linker, so an LD override needs a working CC
out=$(ARCH=arm64 LD=my-own-ld ./build.sh 2>&1)
expect_in "arm64 honours an LD override" "LD:   my-own-ld" "$out"

stubs=$(stub_dir x86_64-elf-gcc x86_64-elf-ld nasm)
out=$(PATH="$stubs:/usr/bin:/bin" ARCH=x86_64 ./build.sh 2>&1)
expect_in "x86_64 prefers the bare metal toolchain" "CC:   x86_64-elf-gcc" "$out"
rm -rf "$stubs"

out=$(ARCH=sparc ./build.sh 2>&1)
expect_in "an unknown ARCH is rejected" "unsupported ARCH" "$out"
if [ "$(ARCH=sparc ./build.sh > /dev/null 2>&1; echo $?)" = "1" ]; then
    ok "an unknown ARCH exits non zero"
else
    no "an unknown ARCH exits non zero"
fi

echo ""
echo "=== Build ==="

if out=$(./build.sh 2>&1); then
    ok "x86_64 builds"
    expect_in "x86_64 reports the kernel size against its limit" "max 15360" "$out"
else
    no "x86_64 builds" "$(echo "$out" | tail -3)"
fi

if out=$(ARCH=arm64 ./build.sh 2>&1); then
    ok "arm64 builds"
    expect_in "arm64 is compiled with strict alignment" "-mstrict-align" "$out"
else
    no "arm64 builds" "$(echo "$out" | tail -3)"
fi

if [ -f os.bin ] && [ "$(wc -c < os.bin | tr -d ' ')" = "32768" ]; then
    ok "the x86_64 image is padded to 32K"
else
    no "the x86_64 image is padded to 32K"
fi

echo ""
echo "=== Shell on x86_64 ==="

./build.sh > /dev/null 2>&1
run_guest x86_64 "help" "echo hello kernie" "bogus"
expect_in "the splash names the architecture" "ARCH  x86_64" "$GUEST_OUT"
expect_in "help lists every command" "shutdown   - power off the machine" "$GUEST_OUT"
expect_in "echo prints its argument" "hello kernie" "$GUEST_OUT"
expect_in "an unknown command is reported" "bogus: command not found" "$GUEST_OUT"

run_guest x86_64 "tick"
expect_in "tick reports a counter" "Ticks: 0x" "$GUEST_OUT"
expect_not_in "the timer is actually running" "Ticks: 0x0000000000000000" "$GUEST_OUT"

# a terminal sending CRLF must not run a second empty command, and a paste
# burst must not be dropped now that the shell runs outside the interrupt
run_guest x86_64 "echo one" "echo b1
echo b2
echo b3"
expect_in "a pasted burst is not dropped" "b3" "$GUEST_OUT"

run_guest x86_64 "shutdown"
if [ "$GUEST_ALIVE" = "no" ]; then
    ok "shutdown powers off x86_64"
else
    no "shutdown powers off x86_64" "qemu was still running"
fi

echo ""
echo "=== Shell on arm64 ==="

ARCH=arm64 ./build.sh > /dev/null 2>&1
run_guest arm64 "help" "echo hello from arm64" "bogus"
expect_in "the splash names the architecture" "ARCH  arm64" "$GUEST_OUT"
expect_in "the splash does not claim a VGA it lacks" "[ CONSOLE OK ]" "$GUEST_OUT"
expect_in "help lists every command" "shutdown   - power off the machine" "$GUEST_OUT"
expect_in "echo prints its argument" "hello from arm64" "$GUEST_OUT"
expect_in "an unknown command is reported" "bogus: command not found" "$GUEST_OUT"

run_guest arm64 "tick"
expect_in "tick reports the stub counter" "Ticks: 0x0000000000000000" "$GUEST_OUT"

run_guest arm64 "reboot" "echo back up"
banners=$(printf '%s' "$GUEST_OUT" | grep -c 'unreasonable ambitions')
if [ "$banners" -ge 2 ]; then
    ok "reboot restarts the arm64 guest"
else
    no "reboot restarts the arm64 guest" "saw $banners banners, expected 2"
fi

# boot.S parks every core but the first, so a second cpu must not reach
# kernel_main and print a banner of its own
QEMU_EXTRA="-smp 2" run_guest arm64 "echo two cpus"
banners=$(printf '%s' "$GUEST_OUT" | grep -c 'unreasonable ambitions')
if [ "$banners" -eq 1 ]; then
    ok "secondary cores are parked"
else
    no "secondary cores are parked" "saw $banners banners, expected 1"
fi
expect_in "the shell still works with two cpus" "two cpus" "$GUEST_OUT"
QEMU_EXTRA=""

# a smoke test of a topology the other cases never reach, not a check of the
# affinity masking itself, which QEMU cannot exercise because it never gives a
# secondary core an Aff0 of zero whatever the declared topology
QEMU_EXTRA="-smp 4,clusters=2,cores=2,threads=1" run_guest arm64 "echo clustered"
banners=$(printf '%s' "$GUEST_OUT" | grep -c 'unreasonable ambitions')
if [ "$banners" -eq 1 ]; then
    ok "a clustered topology boots one kernel"
else
    no "a clustered topology boots one kernel" "saw $banners banners, expected 1"
fi
expect_in "the shell still works in a clustered topology" "clustered" "$GUEST_OUT"
QEMU_EXTRA=""

run_guest arm64 "shutdown"
if [ "$GUEST_ALIVE" = "no" ]; then
    ok "shutdown powers off arm64"
else
    no "shutdown powers off arm64" "qemu was still running"
fi

echo ""
echo "=== Configuration ==="

# a failing suite has to fail the job, and the default shell reports the status
# of tee rather than of the pipeline, which would hide every failure above
expect_in "the test workflow forces a pipefail shell" "shell: bash" \
    "$(sed -n '/- name: Run tests/,/run:/p' .github/workflows/main.yml)"

# the architecture label comes from the build, so that adding a target never
# means editing anything under kernel/
expect_not_in "version.h carries no architecture conditional" "__aarch64__" "$(cat kernel/version.h)"
expect_in "the build supplies the architecture label" "-DKERNIE_ARCH" "$(cat build.sh)"

# the README, the roadmap and AGENTS.md all say the suite runs on Linux, so
# assert the workflow still matches rather than waiting for a reviewer to spot
# the drift
expect_not_in "macOS does not run the QEMU suite" "test.sh" \
    "$(sed -n '/^  build-macos:/,/^  build-toolchain:/p' .github/workflows/main.yml)"
expect_in "the QEMU suite runs on Linux" "runs-on: ubuntu-latest" \
    "$(sed -n '/^  build-test:/,$p' .github/workflows/main.yml)"

if grep -rqE '#(if |ifdef |elif )' kernel lib --include='*.c' --include='*.h'; then
    no "kernel and lib carry no conditional compilation"
else
    ok "kernel and lib carry no conditional compilation"
fi

echo ""
echo "=== Done! ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
[ "$FAIL" -eq 0 ]
