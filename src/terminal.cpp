#include "../include/slowertext.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>
#include <cstring>

Terminal::Terminal() {
    enable_raw_mode();
}

Terminal::~Terminal() {
    disable_raw_mode();
    clear_screen();
    set_cursor_position(0, 0);
    show_cursor();
}

void Terminal::enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

void Terminal::disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

int Terminal::get_window_size(int* rows, int* cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void Terminal::clear_screen() {
    write(STDOUT_FILENO, CLEAR_SCREEN, 4);
    write(STDOUT_FILENO, CURSOR_HOME, 3);
}

void Terminal::set_cursor_position(int x, int y) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y + 1, x + 1);
    write(STDOUT_FILENO, buf, strlen(buf));
}

void Terminal::hide_cursor() {
    write(STDOUT_FILENO, CURSOR_HIDE, 6);
}

void Terminal::show_cursor() {
    write(STDOUT_FILENO, CURSOR_SHOW, 6);
}