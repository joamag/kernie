#include "shell.h"
#include "vga.h"
#include "serial.h"
#include "io.h"

#define CMD_MAX 256

static char cmd_buf[CMD_MAX];
static int cmd_len = 0;

static void print(const char *str, uint8_t color) {
    vga_print(str, color);
    serial_print(str);
}

static void prompt(void) {
    print("> ", VGA_WHITE_ON_BLACK);
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
    print("Available commands:\n", VGA_WHITE_ON_BLACK);
    print("  help       - show this message\n", VGA_WHITE_ON_BLACK);
    print("  clear      - clear the screen\n", VGA_WHITE_ON_BLACK);
    print("  echo <msg> - print a message\n", VGA_WHITE_ON_BLACK);
    print("  tick       - show timer tick count\n", VGA_WHITE_ON_BLACK);
    print("  reboot     - reboot the system\n", VGA_WHITE_ON_BLACK);
}

static void cmd_echo(const char *args) {
    print(args, VGA_WHITE_ON_BLACK);
    print("\n", VGA_WHITE_ON_BLACK);
}

extern volatile uint64_t tick_count;

static void cmd_tick(void) {
    print("Ticks: ", VGA_WHITE_ON_BLACK);
    vga_print_hex(tick_count, VGA_GREEN_ON_BLACK);
    serial_print_hex(tick_count);
    print("\n", VGA_WHITE_ON_BLACK);
}

static void cmd_reboot(void) {
    print("Rebooting...\n", VGA_RED_ON_BLACK);
    /* pulse the keyboard controller reset line */
    outb(0x64, 0xFE);
}

static void execute(void) {
    cmd_buf[cmd_len] = 0;

    if (cmd_len == 0) {
        /* empty command */
    } else if (streq(cmd_buf, "help")) {
        cmd_help();
    } else if (streq(cmd_buf, "clear")) {
        vga_clear();
    } else if (starts_with(cmd_buf, "echo ")) {
        cmd_echo(cmd_buf + 5);
    } else if (streq(cmd_buf, "echo")) {
        print("\n", VGA_WHITE_ON_BLACK);
    } else if (streq(cmd_buf, "tick")) {
        cmd_tick();
    } else if (streq(cmd_buf, "reboot")) {
        cmd_reboot();
    } else {
        print(cmd_buf, VGA_RED_ON_BLACK);
        print(": command not found\n", VGA_RED_ON_BLACK);
    }

    cmd_len = 0;
    prompt();
}

void shell_init(void) {
    print("=== Kernie the Kernel ===\n", VGA_GREEN_ON_BLACK);
    print("Type 'help' for available commands.\n\n", VGA_WHITE_ON_BLACK);
    prompt();
}

void shell_handle_char(char c) {
    if (c == '\n' || c == '\r') {
        print("\n", VGA_WHITE_ON_BLACK);
        execute();
    } else if (c == '\b' || c == 127) {
        if (cmd_len > 0) {
            cmd_len--;
            /* move cursor back, overwrite with space, move back again */
            vga_print("\b \b", VGA_WHITE_ON_BLACK);
            serial_print("\b \b");
        }
    } else if (cmd_len < CMD_MAX - 1) {
        cmd_buf[cmd_len++] = c;
        char str[2] = {c, 0};
        vga_print(str, VGA_WHITE_ON_BLACK);
        serial_putchar(c);
    }
}
