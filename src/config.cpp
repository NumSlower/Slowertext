#include "../include/slowertext.h"
#include <fstream>
#include <sstream>
#include <map>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

/**
 * Load configuration from file and set defaults.
 *
 * BUG FIX: debug_mode was used to gate debug output *before* the config file
 * was parsed, meaning the debug messages about opening the config file were
 * never shown (debug_mode defaulted to false and the config hadn't been read
 * yet).  We now do a two-pass read: first parse the file into a value map,
 * then apply it so that all values — including debug_mode — are available
 * together before any debug printing happens.
 */
void ConfigManager::load_config(EditorConfig& config) {
    // ── Defaults ──────────────────────────────────────────────────────────────
    config.show_line_numbers     = false;
    config.tab_width             = 4;
    config.auto_indent           = true;
    config.show_whitespace       = false;
    config.status_format         = "%f%modified - %m";
    config.text_color            = "white";
    config.background_color      = "black";
    config.status_bar_color      = "cyan";
    config.comment_color         = "green";
    config.show_tilde            = true;
    config.highlight_current_line= false;
    config.confirm_quit          = true;
    config.auto_save_interval    = 0;
    config.create_backups        = false;
    config.max_undo_levels       = 100;
    config.word_wrap             = false;
    config.default_extension     = "txt";
    config.show_hidden_files     = false;
    config.default_encoding      = "utf-8";
    config.line_endings          = "unix";
    config.buffer_size           = 64;
    config.refresh_rate          = 16;
    config.syntax_highlighting   = false;
    config.debug_mode            = false;

    config.enter_insert   = "ctrl+i";
    config.enter_command  = "escape";
    config.save_file      = "ctrl+s";
    config.quit_editor    = "ctrl+q";
    config.force_quit     = "ctrl+f";
    config.cursor_up      = "arrow_up";
    config.cursor_down    = "arrow_down";
    config.cursor_left    = "arrow_left";
    config.cursor_right   = "arrow_right";

    // ── Read config file ──────────────────────────────────────────────────────
    std::string config_path = get_config_path();
    std::ifstream config_file(config_path);

    std::map<std::string, std::string> config_values;

    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) {
            parse_config_line(line, config_values);
        }
        config_file.close();
    }

    // Apply all values at once (so debug_mode is set before we use it)
    apply_config_values(config, config_values);

    // ── Post-apply debug output ───────────────────────────────────────────────
    if (config.debug_mode) {
        if (config_file.good() || !config_values.empty()) {
            std::cerr << "Debug: Loaded config from: " << config_path << '\n';
        } else {
            std::cerr << "Debug: Could not open config file: " << config_path << '\n';
            std::cerr << "Debug: Using default configuration values\n";
        }
        std::cerr << "Debug: Found " << config_values.size() << " configuration values\n";
        std::cerr << "Debug: Final tab_width = " << config.tab_width << '\n';
    }
}

/**
 * Return the path of the first config file that exists, checking locations in
 * priority order.  Falls back to the canonical user-config path if none found.
 */
std::string ConfigManager::get_config_path() {
    const char* home_env = getenv("HOME");
    std::string home_dir;
    if (home_env) {
        home_dir = home_env;
    } else {
        struct passwd* pw = getpwuid(getuid());
        home_dir = pw ? pw->pw_dir : ".";
    }

    std::vector<std::string> candidates = {
        home_dir + "/.config/slowertext/slowertextrc",
        home_dir + "/.slowertextrc",
        "runtime/slowertextrc",
        "/etc/slowertext/slowertextrc"
    };

    for (const auto& p : candidates) {
        if (FileManager::file_exists(p)) return p;
    }

    return home_dir + "/.config/slowertext/slowertextrc";
}

/**
 * Parse one config line into the values map.
 * Ignores blank lines, comments, and section headers.
 * Strips inline comments (anything after '#' that is preceded by whitespace).
 */
void ConfigManager::parse_config_line(const std::string& line,
                                      std::map<std::string, std::string>& values) {
    if (line.empty() || line[0] == '#' || line[0] == '[') return;

    size_t eq = line.find('=');
    if (eq == std::string::npos) return;

    std::string key   = line.substr(0, eq);
    std::string value = line.substr(eq + 1);

    // Trim key
    key.erase(0, key.find_first_not_of(" \t"));
    auto key_end = key.find_last_not_of(" \t");
    if (key_end != std::string::npos) key.erase(key_end + 1);

    // Trim value leading whitespace
    value.erase(0, value.find_first_not_of(" \t"));

    // Strip inline comment: everything from the first ' #' or '\t#' onwards
    // BUG FIX: the original code did not strip inline comments, meaning lines
    // like "tab_width = 3   # comment" would set tab_width to "3   # comment",
    // causing stoi() to throw (and the setting to be silently ignored).
    size_t comment_pos = value.find(" #");
    if (comment_pos == std::string::npos) comment_pos = value.find("\t#");
    if (comment_pos != std::string::npos) value.erase(comment_pos);

    // Trim value trailing whitespace
    auto val_end = value.find_last_not_of(" \t");
    if (val_end != std::string::npos) value.erase(val_end + 1);
    else value.clear();

    // Remove surrounding quotes
    if (value.size() >= 2 &&
        ((value.front() == '"'  && value.back() == '"')  ||
         (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }

    if (!key.empty()) values[key] = value;
}

/**
 * Apply a map of parsed config values to the editor config struct.
 */
void ConfigManager::apply_config_values(EditorConfig& config,
                                        const std::map<std::string, std::string>& values) {
    for (const auto& kv : values) {
        const std::string& key   = kv.first;
        const std::string& value = kv.second;

        try {
            // Display
            if      (key == "show_line_numbers")      config.show_line_numbers      = string_to_bool(value);
            else if (key == "tab_width") {
                int v = std::stoi(value);
                if (v > 0 && v <= 16) config.tab_width = v;
            }
            else if (key == "auto_indent")            config.auto_indent            = string_to_bool(value);
            else if (key == "show_whitespace")        config.show_whitespace        = string_to_bool(value);
            else if (key == "status_format")          config.status_format          = value;
            else if (key == "text_color")             config.text_color             = value;
            else if (key == "background_color")       config.background_color       = value;
            else if (key == "status_bar_color")       config.status_bar_color       = value;
            else if (key == "comment_color")          config.comment_color          = value;
            else if (key == "show_tilde")             config.show_tilde             = string_to_bool(value);
            else if (key == "highlight_current_line") config.highlight_current_line = string_to_bool(value);
            // Behaviour
            else if (key == "confirm_quit")           config.confirm_quit           = string_to_bool(value);
            else if (key == "auto_save_interval") {
                int v = std::stoi(value);
                if (v >= 0) config.auto_save_interval = v;
            }
            else if (key == "create_backups")         config.create_backups         = string_to_bool(value);
            else if (key == "max_undo_levels") {
                int v = std::stoi(value);
                if (v > 0) config.max_undo_levels = v;
            }
            else if (key == "word_wrap")              config.word_wrap              = string_to_bool(value);
            else if (key == "default_extension")      config.default_extension      = value;
            else if (key == "show_hidden_files")      config.show_hidden_files      = string_to_bool(value);
            else if (key == "default_encoding")       config.default_encoding       = value;
            else if (key == "line_endings")           config.line_endings           = value;
            else if (key == "buffer_size") {
                int v = std::stoi(value);
                if (v > 0) config.buffer_size = v;
            }
            else if (key == "refresh_rate") {
                int v = std::stoi(value);
                if (v > 0) config.refresh_rate = v;
            }
            else if (key == "syntax_highlighting")    config.syntax_highlighting    = string_to_bool(value);
            else if (key == "debug_mode")             config.debug_mode             = string_to_bool(value);
            // Default mode
            else if (key == "default_mode") {
                if      (value == "insert")  config.mode = INSERT_MODE;
                else if (value == "command") config.mode = COMMAND_MODE;
            }
            // Key bindings
            else if (key == "enter_insert")   config.enter_insert   = value;
            else if (key == "enter_command")  config.enter_command  = value;
            else if (key == "save_file")      config.save_file      = value;
            else if (key == "quit_editor")    config.quit_editor    = value;
            else if (key == "force_quit")     config.force_quit     = value;
            else if (key == "cursor_up")      config.cursor_up      = value;
            else if (key == "cursor_down")    config.cursor_down    = value;
            else if (key == "cursor_left")    config.cursor_left    = value;
            else if (key == "cursor_right")   config.cursor_right   = value;
            // Silently ignore unknown keys (e.g. "font")

        } catch (const std::exception& e) {
            if (config.debug_mode) {
                std::cerr << "Debug: Error parsing config key '" << key
                          << "': " << e.what() << '\n';
            }
        }
    }
}

/**
 * Convert a string to bool.  Accepts true/1/yes/on (case-insensitive).
 */
bool ConfigManager::string_to_bool(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return (s == "true" || s == "1" || s == "yes" || s == "on");
}

/**
 * Parse a key-binding string into an integer key code.
 */
int ConfigManager::parse_key_binding(const std::string& key) {
    if (key.empty()) return 0;

    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    if (k == "escape" || k == "esc")               return ESC_KEY;
    if (k == "backspace")                           return BACKSPACE_KEY;
    if (k == "delete"  || k == "del")              return DELETE_KEY;
    if (k == "arrow_up"    || k == "up")           return ARROW_UP;
    if (k == "arrow_down"  || k == "down")         return ARROW_DOWN;
    if (k == "arrow_left"  || k == "left")         return ARROW_LEFT;
    if (k == "arrow_right" || k == "right")        return ARROW_RIGHT;
    if (k == "tab")                                 return '\t';
    if (k == "enter" || k == "return")             return '\r';
    if (k == "space")                               return ' ';

    // ctrl+<letter>
    if (k.size() == 6 && k.substr(0, 5) == "ctrl+") {
        char ch = k[5];
        if (ch >= 'a' && ch <= 'z') return CTRL_KEY(ch);
    }

    // Single printable character
    if (k.size() == 1) return k[0];

    return 0;
}