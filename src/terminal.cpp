#include "../include/slowertext.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>
#include <cstring>
#include <cerrno>

/**
 * Terminal constructor
 * Enables raw mode for character-by-character input
 */
Terminal::Terminal() {
    enable_raw_mode();
}

/**
 * Terminal destructor
 * Restores original terminal settings and cleans up display
 */
Terminal::~Terminal() {
    disable_raw_mode();
    clear_screen();
    set_cursor_position(0, 0);
    show_cursor();
}

/**
 * Enable raw terminal mode for immediate key input
 * Disables canonical mode, echo, and various control sequences
 */
void Terminal::enable_raw_mode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    struct termios raw = orig_termios;

    // Input modes: disable break signal, CR-to-NL, parity check,
    // strip 8th bit, and XON/XOFF flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Output modes: disable post-processing
    raw.c_oflag &= ~(OPOST);

    // Control modes: set 8-bit character size
    raw.c_cflag |= (CS8);

    // Local modes: disable echo, canonical mode, extended functions, signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Control chars: min bytes for non-blocking read, timeout in deciseconds
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

/**
 * Disable raw mode and restore original terminal settings
 */
void Terminal::disable_raw_mode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) {
        perror("tcsetattr");
        // Don't exit here — we're already cleaning up
    }
}

/**
 * Get terminal window size.
 * Primary method: ioctl TIOCGWINSZ.
 * Fallback: move cursor to bottom-right corner and query its position.
 *
 * @param rows  Output — number of terminal rows
 * @param cols  Output — number of terminal columns
 * @return 0 on success, -1 on error
 */
int Terminal::get_window_size(int* rows, int* cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

    // --- Fallback: push cursor to bottom-right then read its position ---
    // Move cursor far right and far down
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
        return -1;
    }

    // Request cursor position report: ESC [ 6 n
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    // Read the response: ESC [ <rows> ; <cols> R
    char buf[32];
    unsigned int i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

/**
 * Clear entire screen and move cursor to home position
 */
void Terminal::clear_screen() {
    twrite(STDOUT_FILENO, CLEAR_SCREEN, CLEAR_SCREEN_LEN);
    twrite(STDOUT_FILENO, CURSOR_HOME,  CURSOR_HOME_LEN);
}

/**
 * Set cursor to a specific position (0-based x, y converted to 1-based ANSI)
 */
void Terminal::set_cursor_position(int x, int y) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y + 1, x + 1);
    if (len > 0) {
        twrite(STDOUT_FILENO, buf, static_cast<size_t>(len));
    }
}

/**
 * Hide cursor from display
 */
void Terminal::hide_cursor() {
    twrite(STDOUT_FILENO, CURSOR_HIDE, CURSOR_HIDE_LEN);
}

/**
 * Show cursor on display
 */
void Terminal::show_cursor() {
    twrite(STDOUT_FILENO, CURSOR_SHOW, CURSOR_SHOW_LEN);
}