#ifndef SLOWERTEXT_H
#define SLOWERTEXT_H

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <csignal>
#include <map>
#include <ctime>

// ANSI escape codes for terminal control
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define CLEAR_LINE "\033[K"
#define COLOR_RESET "\033[m"

// Text colors
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// Background colors
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

// ANSI escape code lengths (for write() calls)
#define CLEAR_SCREEN_LEN 4
#define CURSOR_HOME_LEN  3
#define CURSOR_HIDE_LEN  6
#define CURSOR_SHOW_LEN  6
#define CLEAR_LINE_LEN   3
#define COLOR_RESET_LEN  3

// Key codes for input handling
#define CTRL_KEY(k) ((k) & 0x1f)
#define ESC_KEY 27
#define BACKSPACE_KEY 127
#define DELETE_KEY 1000
#define ARROW_UP 1001
#define ARROW_DOWN 1002
#define ARROW_LEFT 1003
#define ARROW_RIGHT 1004

/**
 * Editor mode enumeration
 * INSERT_MODE: Normal text editing mode
 * COMMAND_MODE: Vi-like command mode for navigation and commands
 */
enum EditorMode {
    INSERT_MODE,
    COMMAND_MODE
};

/**
 * Main configuration structure for the editor
 * Contains all editor settings, display options, and key bindings
 */
struct EditorConfig {
    // Screen and cursor state
    int screen_rows;           // Number of rows available for text display
    int screen_cols;           // Number of columns available for text display
    int cursor_x;              // Current cursor column position
    int cursor_y;              // Current cursor row position
    int row_offset;            // Vertical scroll offset
    int col_offset;            // Horizontal scroll offset

    // Editor state
    EditorMode mode;           // Current editor mode (INSERT or COMMAND)
    std::string filename;      // Currently opened file name
    bool modified;             // Whether the buffer has been modified
    bool quit;                 // Flag to exit the editor
    std::string status_msg;    // Current status message
    time_t status_msg_time;    // When the status message was set

    // Display configuration
    bool show_line_numbers;    // Whether to display line numbers
    int tab_width;             // Number of spaces for tab character
    bool auto_indent;          // Auto-indent new lines
    bool show_whitespace;      // Show whitespace characters (unused)
    std::string status_format; // Format string for status bar
    bool show_tilde;           // Show tilde for empty lines
    bool highlight_current_line; // Highlight the current line

    // Color configuration
    std::string text_color;        // Text color
    std::string background_color;  // Background color
    std::string status_bar_color;  // Status bar color
    std::string comment_color;     // Comment text color

    // Editor behavior
    bool confirm_quit;         // Confirm before quitting with unsaved changes
    int auto_save_interval;    // Auto-save interval (unused)
    bool create_backups;       // Create backup files (unused)
    int max_undo_levels;       // Maximum undo levels (unused)
    bool word_wrap;            // Word wrap (unused)
    std::string default_extension; // Default file extension
    bool show_hidden_files;    // Show hidden files (unused)
    std::string default_encoding;  // Default text encoding (unused)
    std::string line_endings;  // Line ending style (unused)
    int buffer_size;           // Buffer size (unused)
    int refresh_rate;          // Screen refresh rate (unused)
    bool syntax_highlighting;  // Enable basic syntax highlighting
    bool debug_mode;           // Enable debug messages

    // Key bindings (stored as strings for configuration)
    std::string enter_insert;   // Key to enter insert mode
    std::string enter_command;  // Key to enter command mode
    std::string save_file;      // Key to save file
    std::string quit_editor;    // Key to quit editor
    std::string force_quit;     // Key to force quit
    std::string cursor_up;      // Key for cursor up
    std::string cursor_down;    // Key for cursor down
    std::string cursor_left;    // Key for cursor left
    std::string cursor_right;   // Key for cursor right
};

/**
 * Configuration manager class
 * Handles loading and parsing of configuration files
 */
class ConfigManager {
public:
    static void load_config(EditorConfig& config);
    static std::string get_config_path();
    static void parse_config_line(const std::string& line, std::map<std::string, std::string>& values);
    static void apply_config_values(EditorConfig& config, const std::map<std::string, std::string>& values);
    static bool string_to_bool(const std::string& str);
    static int parse_key_binding(const std::string& key);
};

/**
 * Text buffer class
 * Manages the text content and modifications
 */
class Buffer {
private:
    std::vector<std::string> lines;
    bool modified;

public:
    Buffer();
    void insert_char(int x, int y, char c);
    void delete_char(int x, int y);
    void insert_newline(int x, int y);
    void delete_line(int y);
    std::string get_line(int y) const;
    int get_line_count() const;
    void set_line(int y, const std::string& line);
    bool is_modified() const;
    void set_modified(bool mod);
    void clear();
    const std::vector<std::string>& get_lines() const;
};

/**
 * File operations manager
 */
class FileManager {
public:
    static bool load_file(const std::string& filename, Buffer& buffer);
    static bool save_file(const std::string& filename, const Buffer& buffer);
    static bool file_exists(const std::string& filename);
};

/**
 * Terminal control class
 */
class Terminal {
private:
    struct termios orig_termios;

public:
    Terminal();
    ~Terminal();
    void enable_raw_mode();
    void disable_raw_mode();
    int get_window_size(int* rows, int* cols);
    void clear_screen();
    void set_cursor_position(int x, int y);
    void hide_cursor();
    void show_cursor();
};

/**
 * Input handling class
 */
class InputHandler {
public:
    static int read_key();
    static void process_keypress(EditorConfig& config, Buffer& buffer);
    static void process_command(EditorConfig& config, Buffer& buffer, const std::string& command);
};

/**
 * Screen rendering class
 */
class Renderer {
public:
    static void draw_rows(const EditorConfig& config, const Buffer& buffer);
    static void draw_status_bar(const EditorConfig& config, const Buffer& buffer);
    static void draw_message_bar(const EditorConfig& config);
    static void refresh_screen(const EditorConfig& config, const Buffer& buffer);
    static void scroll(EditorConfig& config, const Buffer& buffer);
};

// Global instances
extern EditorConfig editor_config;
extern Terminal terminal;

// Utility functions
void handle_sigwinch(int sig);
void cleanup_and_exit();
void set_status_message(const std::string& msg);

#endif

// ── Terminal write helper ────────────────────────────────────────────────────
// write(2) is declared with [[nodiscard]] / warn_unused_result on some toolchains.
// Using this wrapper avoids the warning across the codebase without -Wno-* flags.
#include <cstddef>
inline void twrite(int fd, const void* buf, std::size_t n) {
    // We intentionally ignore the return value: partial writes to a terminal
    // are rare and non-recoverable in a TUI editor context.
    if (write(fd, buf, n)) { /* suppress nodiscard */ }
}