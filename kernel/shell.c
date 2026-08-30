/**
 * kernel/shell.c
 *
 * Line buffered command shell.
 *
 * Runs entirely outside interrupt context, driven by input_poll from the idle
 * loop, so a command is free to take as long as it needs to write its output.
 *
 * Commands are dispatched by a chain of comparisons rather than a table. With
 * six of them the table would cost more to read than it saves, and the point
 * at which that stops being true is obvious enough to act on when it arrives.
 *
 * Reboot and power off are delegated to the architecture, since one is a
 * keyboard controller pulse and the other an ACPI write on x86-64, while both
 * are PSCI calls on arm64.
 *
 * A line feed arriving straight after a carriage return is swallowed, so that
 * a terminal configured to send the pair does not run a second, empty
 * command.
 */

#include "kernel/shell.h"
#include "kernel/console.h"
#include "kernel/arch.h"
#include "kernel/version.h"

#define CMD_MAX 256

static char cmd_buf[CMD_MAX];
static int cmd_len = 0;
static int last_was_cr = 0;

static void print(const char *str, uint8_t color) {
    console_print(str, color);
}

static void prompt(void) {
    print("> ", CONSOLE_WHITE);
}

static void splash(void) {
    print("  --------------------------------------------------------------------------\n",
          CONSOLE_BLUE);
    print("\n", CONSOLE_WHITE);

    // Folded-K symbol and wordmark, kept below 80 columns for VGA text mode.
    print("        ||      ", CONSOLE_BLUE);
    print("//////      ", CONSOLE_CYAN);
    print("_  __ _____ ____  _   _ ___ _____\n", CONSOLE_WHITE);

    print("        ||    ", CONSOLE_BLUE);
    print("//////        ", CONSOLE_CYAN);
    print("| |/ /| ____|  _ \\| \\ | |_ _| ____|\n", CONSOLE_WHITE);

    print("        ||", CONSOLE_BLUE);
    print("<<                ", CONSOLE_CYAN);
    print("| ' / |  _| | |_) |  \\| || ||  _|\n", CONSOLE_WHITE);

    print("        ||    ", CONSOLE_BLUE);
    print("\\\\\\\\\\\\        ", CONSOLE_MAGENTA);
    print("| . \\ | |___|  _ <| |\\  || || |___\n", CONSOLE_WHITE);

    print("        ||      ", CONSOLE_BLUE);
    print("\\\\\\\\\\\\      ", CONSOLE_MAGENTA);
    print("|_|\\_\\|_____|_| \\_\\_| \\_|___|_____|\n", CONSOLE_WHITE);

    print("\n", CONSOLE_WHITE);
    print("               A tiny kernel with unreasonable ambitions.\n",
          CONSOLE_GREEN);
    print("\n", CONSOLE_WHITE);
    print("  --------------------------------------------------------------------------\n",
          CONSOLE_BLUE);

    print("      VERSION  ", CONSOLE_GRAY);
    print(KERNIE_VERSION, CONSOLE_YELLOW);
    print("    ARCH  ", CONSOLE_GRAY);
    print(KERNIE_ARCH, CONSOLE_CYAN);
    print("    BUILT  ", CONSOLE_GRAY);
    print(KERNIE_BUILD_DATE, CONSOLE_WHITE);
    print(" ", CONSOLE_WHITE);
    print(KERNIE_BUILD_TIME, CONSOLE_WHITE);
    print("\n", CONSOLE_WHITE);

    print("             [ CONSOLE OK ]  [ SERIAL OK ]  [ SHELL READY ]\n",
          CONSOLE_GREEN);
    print("  --------------------------------------------------------------------------\n",
          CONSOLE_BLUE);
    print("\n", CONSOLE_WHITE);
}

static int streq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a++ != *b++) return 0;
    }
    return *a == *b;
}

static int starts_with(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

static void cmd_help(void) {
    print("Available commands:\n", CONSOLE_WHITE);
    print("  help       - show this message\n", CONSOLE_WHITE);
    print("  clear      - clear the screen\n", CONSOLE_WHITE);
    print("  echo <msg> - print a message\n", CONSOLE_WHITE);
    print("  tick       - show timer tick count\n", CONSOLE_WHITE);
    print("  reboot     - reboot the system\n", CONSOLE_WHITE);
    print("  shutdown   - power off the machine\n", CONSOLE_WHITE);
}

static void cmd_echo(const char *args) {
    print(args, CONSOLE_WHITE);
    print("\n", CONSOLE_WHITE);
}

static void cmd_tick(void) {
    print("Ticks: ", CONSOLE_WHITE);
    console_print_hex(arch_ticks(), CONSOLE_GREEN);
    print("\n", CONSOLE_WHITE);
}

static void cmd_reboot(void) {
    print("Rebooting...\n", CONSOLE_RED);
    arch_reboot();
}

static void cmd_shutdown(void) {
    print("Shutting down...\n", CONSOLE_RED);
    arch_shutdown();
}

static void execute(void) {
    cmd_buf[cmd_len] = 0;

    if (cmd_len == 0) {
        // empty command
    } else if (streq(cmd_buf, "help")) {
        cmd_help();
    } else if (streq(cmd_buf, "clear")) {
        console_clear();
    } else if (starts_with(cmd_buf, "echo ")) {
        cmd_echo(cmd_buf + 5);
    } else if (streq(cmd_buf, "echo")) {
        print("\n", CONSOLE_WHITE);
    } else if (streq(cmd_buf, "tick")) {
        cmd_tick();
    } else if (streq(cmd_buf, "reboot")) {
        cmd_reboot();
    } else if (streq(cmd_buf, "shutdown")) {
        cmd_shutdown();
    } else {
        print(cmd_buf, CONSOLE_RED);
        print(": command not found\n", CONSOLE_RED);
    }

    cmd_len = 0;
    prompt();
}

void shell_init(void) {
    splash();
    print("  Type 'help' for available commands.\n\n", CONSOLE_WHITE);
    prompt();
}

void shell_handle_char(char c) {
    // swallow the LF of a CRLF pair, it is not a second command
    if (c == '\n' && last_was_cr) {
        last_was_cr = 0;
        return;
    }

    last_was_cr = (c == '\r');

    if (c == '\n' || c == '\r') {
        print("\n", CONSOLE_WHITE);
        execute();
    } else if (c == '\b' || c == 127) {
        if (cmd_len > 0) {
            cmd_len--;
            // move cursor back, overwrite with space, move back again
            print("\b \b", CONSOLE_WHITE);
        }
    } else if (cmd_len < CMD_MAX - 1) {
        cmd_buf[cmd_len++] = c;
        char str[2] = {c, 0};
        print(str, CONSOLE_WHITE);
    }
}
