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
    /**
     * Load configuration from file into editor config
     * @param config Reference to editor configuration to populate
     */
    static void load_config(EditorConfig& config);
    
    /**
     * Get the path to the configuration file
     * @return Path to configuration file
     */
    static std::string get_config_path();
    
    /**
     * Parse a single configuration line
     * @param line Configuration line to parse
     * @param values Map to store parsed key-value pairs
     */
    static void parse_config_line(const std::string& line, std::map<std::string, std::string>& values);
    
    /**
     * Apply parsed configuration values to editor config
     * @param config Editor configuration to update
     * @param values Map of configuration values
     */
    static void apply_config_values(EditorConfig& config, const std::map<std::string, std::string>& values);
    
    /**
     * Convert string to boolean value
     * @param str String to convert
     * @return Boolean value
     */
    static bool string_to_bool(const std::string& str);
    
    /**
     * Parse key binding string to key code
     * @param key Key binding string
     * @return Key code integer
     */
    static int parse_key_binding(const std::string& key);
};

/**
 * Text buffer class
 * Manages the text content and modifications
 */
class Buffer {
private:
    std::vector<std::string> lines;  // Text lines
    bool modified;                   // Modification flag

public:
    /**
     * Constructor - initializes empty buffer
     */
    Buffer();
    
    /**
     * Insert a character at specified position
     * @param x Column position
     * @param y Row position
     * @param c Character to insert
     */
    void insert_char(int x, int y, char c);
    
    /**
     * Delete character at specified position
     * @param x Column position
     * @param y Row position
     */
    void delete_char(int x, int y);
    
    /**
     * Insert a new line at specified position
     * @param x Column position where to split
     * @param y Row position
     */
    void insert_newline(int x, int y);
    
    /**
     * Delete entire line
     * @param y Row position
     */
    void delete_line(int y);
    
    /**
     * Get text content of a line
     * @param y Row position
     * @return Line content as string
     */
    std::string get_line(int y) const;
    
    /**
     * Get total number of lines in buffer
     * @return Number of lines
     */
    int get_line_count() const;
    
    /**
     * Set content of a specific line
     * @param y Row position
     * @param line New line content
     */
    void set_line(int y, const std::string& line);
    
    /**
     * Check if buffer has been modified
     * @return True if modified
     */
    bool is_modified() const;
    
    /**
     * Set modification flag
     * @param mod New modification state
     */
    void set_modified(bool mod);
    
    /**
     * Clear all buffer content
     */
    void clear();
    
    /**
     * Get reference to all lines for iteration
     * @return Const reference to lines vector
     */
    const std::vector<std::string>& get_lines() const;
};

/**
 * File operations manager
 * Handles loading and saving of files
 */
class FileManager {
public:
    /**
     * Load file content into buffer
     * @param filename File to load
     * @param buffer Buffer to populate
     * @return True if successful
     */
    static bool load_file(const std::string& filename, Buffer& buffer);
    
    /**
     * Save buffer content to file
     * @param filename File to save to
     * @param buffer Buffer to save
     * @return True if successful
     */
    static bool save_file(const std::string& filename, const Buffer& buffer);
    
    /**
     * Check if file exists
     * @param filename File to check
     * @return True if file exists
     */
    static bool file_exists(const std::string& filename);
};

/**
 * Terminal control class
 * Handles raw mode and terminal operations
 */
class Terminal {
private:
    struct termios orig_termios;  // Original terminal settings

public:
    /**
     * Constructor - enables raw mode
     */
    Terminal();
    
    /**
     * Destructor - restores terminal settings
     */
    ~Terminal();
    
    /**
     * Enable raw terminal mode for character input
     */
    void enable_raw_mode();
    
    /**
     * Disable raw mode and restore original settings
     */
    void disable_raw_mode();
    
    /**
     * Get terminal window size
     * @param rows Pointer to store row count
     * @param cols Pointer to store column count
     * @return 0 on success, -1 on error
     */
    int get_window_size(int* rows, int* cols);
    
    /**
     * Clear entire screen
     */
    void clear_screen();
    
    /**
     * Set cursor to specific position
     * @param x Column position
     * @param y Row position
     */
    void set_cursor_position(int x, int y);
    
    /**
     * Hide cursor
     */
    void hide_cursor();
    
    /**
     * Show cursor
     */
    void show_cursor();
};

/**
 * Input handling class
 * Processes keyboard input and commands
 */
class InputHandler {
public:
    /**
     * Read a single key from input
     * @return Key code or -1 on error
     */
    static int read_key();
    
    /**
     * Process a keypress and update editor state
     * @param config Editor configuration
     * @param buffer Text buffer
     */
    static void process_keypress(EditorConfig& config, Buffer& buffer);
    
    /**
     * Process a command mode command
     * @param config Editor configuration
     * @param buffer Text buffer
     * @param command Command string to process
     */
    static void process_command(EditorConfig& config, Buffer& buffer, const std::string& command);
};

/**
 * Screen rendering class
 * Handles all display operations
 */
class Renderer {
public:
    /**
     * Draw text rows on screen
     * @param config Editor configuration
     * @param buffer Text buffer
     */
    static void draw_rows(const EditorConfig& config, const Buffer& buffer);
    
    /**
     * Draw status bar
     * @param config Editor configuration
     * @param buffer Text buffer
     */
    static void draw_status_bar(const EditorConfig& config, const Buffer& buffer);
    
    /**
     * Draw message bar
     * @param config Editor configuration
     */
    static void draw_message_bar(const EditorConfig& config);
    
    /**
     * Refresh entire screen
     * @param config Editor configuration
     * @param buffer Text buffer
     */
    static void refresh_screen(const EditorConfig& config, const Buffer& buffer);
    
    /**
     * Handle scrolling logic
     * @param config Editor configuration (modified)
     * @param buffer Text buffer
     */
    static void scroll(EditorConfig& config, const Buffer& buffer);
};

// Global instances
extern EditorConfig editor_config;  // Global editor configuration
extern Terminal terminal;           // Global terminal instance

// Signal handlers and utility functions
/**
 * Handle window resize signal
 * @param sig Signal number
 */
void handle_sigwinch(int sig);

/**
 * Cleanup resources and exit gracefully
 */
void cleanup_and_exit();

/**
 * Set status message with timestamp
 * @param msg Message to display
 */
void set_status_message(const std::string& msg);

#endif