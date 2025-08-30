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

// ANSI escape codes
#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define CLEAR_LINE "\033[K"
#define COLOR_RESET "\033[m"
#define COLOR_BLACK "\033[30m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

// Key codes
#define CTRL_KEY(k) ((k) & 0x1f)
#define ESC_KEY 27
#define BACKSPACE_KEY 127
#define DELETE_KEY 1000
#define ARROW_UP 1001
#define ARROW_DOWN 1002
#define ARROW_LEFT 1003
#define ARROW_RIGHT 1004

enum EditorMode {
    INSERT_MODE,
    COMMAND_MODE
};

struct EditorConfig {
    int screen_rows;
    int screen_cols;
    int cursor_x;
    int cursor_y;
    int row_offset;
    int col_offset;
    EditorMode mode;
    std::string filename;
    bool modified;
    bool quit;
    std::string status_msg;
    time_t status_msg_time;
    
    // Configuration options from RC file
    bool show_line_numbers;
    int tab_width;
    bool auto_indent;
    bool show_whitespace;
    std::string status_format;
    std::string color_scheme; // Not used directly, but stored for reference
    std::string text_color;
    std::string background_color;
    std::string status_bar_color;
    std::string comment_color;
    std::string font; // Terminal font (informational, depends on terminal)
    bool show_tilde;
    bool highlight_current_line;
    bool confirm_quit;
    int auto_save_interval;
    bool create_backups;
    int max_undo_levels;
    bool word_wrap;
    std::string default_extension;
    bool show_hidden_files;
    std::string default_encoding;
    std::string line_endings;
    int buffer_size;
    int refresh_rate;
    bool syntax_highlighting;
    bool debug_mode;
    
    // Key bindings
    std::string enter_insert;
    std::string enter_command;
    std::string save_file;
    std::string quit_editor;
    std::string force_quit;
    std::string cursor_up;
    std::string cursor_down;
    std::string cursor_left;
    std::string cursor_right;
};

class ConfigManager {
public:
    static void load_config(EditorConfig& config);
    static std::string get_config_path();
    static void parse_config_line(const std::string& line, std::map<std::string, std::string>& values);
    static void apply_config_values(EditorConfig& config, const std::map<std::string, std::string>& values);
    static bool string_to_bool(const std::string& str);
    static int parse_key_binding(const std::string& key);
};

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

class FileManager {
public:
    static bool load_file(const std::string& filename, Buffer& buffer);
    static bool save_file(const std::string& filename, const Buffer& buffer);
    static bool file_exists(const std::string& filename);
};

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

class InputHandler {
public:
    static int read_key();
    static void process_keypress(EditorConfig& config, Buffer& buffer);
    static void process_command(EditorConfig& config, Buffer& buffer, const std::string& command);
};

class Renderer {
public:
    static void draw_rows(const EditorConfig& config, const Buffer& buffer);
    static void draw_status_bar(const EditorConfig& config, const Buffer& buffer);
    static void draw_message_bar(const EditorConfig& config);
    static void refresh_screen(const EditorConfig& config, const Buffer& buffer);
    static void scroll(EditorConfig& config, const Buffer& buffer);
};

// Global editor configuration
extern EditorConfig editor_config;
extern Terminal terminal;

// Signal handler
void handle_sigwinch(int sig);
void cleanup_and_exit();

#endif