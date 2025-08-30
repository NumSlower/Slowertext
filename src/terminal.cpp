#include "../include/slowertext.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <cstdlib>
#include <cstring>

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
    // Save original terminal attributes
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    struct termios raw = orig_termios;
    
    // Input modes: disable break signal, CR to NL conversion, parity check,
    // strip 8th bit, and XON/XOFF flow control
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    
    // Output modes: disable post-processing
    raw.c_oflag &= ~(OPOST);
    
    // Control modes: set 8-bit character size
    raw.c_cflag |= (CS8);
    
    // Local modes: disable echo, canonical mode, extended functions, and signals
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // Control characters: minimum bytes to read and timeout
    raw.c_cc[VMIN] = 0;   // Minimum bytes for non-blocking read
    raw.c_cc[VTIME] = 1;  // Timeout in deciseconds

    // Apply the new terminal settings
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
        exit(1);
    }
}

/**
 * Get terminal window size
 * @param rows Pointer to store number of rows
 * @param cols Pointer to store number of columns
 * @return 0 on success, -1 on error
 */
int Terminal::get_window_size(int* rows, int* cols) {
    struct winsize ws;

    // Try to get window size using ioctl
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // Fallback method: move cursor to bottom-right and get position
        // This is a more complex approach that would require cursor position queries
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return -1; // Simplified - would need full implementation
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/**
 * Clear entire screen and move cursor to home position
 */
void Terminal::clear_screen() {
    write(STDOUT_FILENO, CLEAR_SCREEN, 4);  // Clear screen
    write(STDOUT_FILENO, CURSOR_HOME, 3);   // Move cursor to home
}

/**
 * Set cursor to specific position
 * @param x Column position (0-based)
 * @param y Row position (0-based)
 */
void Terminal::set_cursor_position(int x, int y) {
    char buf[32];
    // ANSI escape sequence for cursor positioning (1-based)
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", y + 1, x + 1);
    write(STDOUT_FILENO, buf, strlen(buf));
}

/**
 * Hide cursor from display
 */
void Terminal::hide_cursor() {
    write(STDOUT_FILENO, CURSOR_HIDE, 6);
}

/**
 * Show cursor on display
 */
void Terminal::show_cursor() {
    write(STDOUT_FILENO, CURSOR_SHOW, 6);
}