#include "../include/slowertext.h"
#include <fstream>
#include <sstream>
#include <map>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>

void ConfigManager::load_config(EditorConfig& config) {
    // Set defaults
    config.show_line_numbers = false;
    config.tab_width = 4;
    config.auto_indent = true;
    config.show_whitespace = false;
    config.status_format = "%f%modified - %m";
    config.color_scheme = "default";
    config.text_color = "white";
    config.background_color = "black";
    config.status_bar_color = "cyan";
    config.comment_color = "green";
    config.font = "monospace";
    config.show_tilde = true;
    config.highlight_current_line = false;
    config.confirm_quit = true;
    config.auto_save_interval = 0;
    config.create_backups = false;
    config.max_undo_levels = 100;
    config.word_wrap = false;
    config.default_extension = "txt";
    config.show_hidden_files = false;
    config.default_encoding = "utf-8";
    config.line_endings = "unix";
    config.buffer_size = 64;
    config.refresh_rate = 16;
    config.syntax_highlighting = false;
    config.debug_mode = false;
    config.enter_insert = "ctrl+i";
    config.enter_command = "escape";
    config.save_file = "ctrl+s";
    config.quit_editor = "ctrl+q";
    config.force_quit = "ctrl+f";
    config.cursor_up = "arrow_up";
    config.cursor_down = "arrow_down";
    config.cursor_left = "arrow_left";
    config.cursor_right = "arrow_right";
    
    std::string config_path = get_config_path();
    std::ifstream config_file(config_path);
    
    if (!config_file.is_open()) {
        return; // Use defaults if config file doesn't exist
    }
    
    std::map<std::string, std::string> config_values;
    std::string line;
    
    while (std::getline(config_file, line)) {
        parse_config_line(line, config_values);
    }
    
    config_file.close();
    apply_config_values(config, config_values);
}

std::string ConfigManager::get_config_path() {
    const char* home = getenv("HOME");
    std::string home_dir;
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) {
            home_dir = pw->pw_dir;
        } else {
            home_dir = "."; // Fallback to current directory
        }
    } else {
        home_dir = home;
    }
    
    // Check configuration file locations in order of priority
    // 1. runtime/slowertextrc
    std::string runtime_path = "runtime/slowertextrc";
    if (FileManager::file_exists(runtime_path)) {
        return runtime_path;
    }
    
    // 2. ~/.config/slowertext/slowertextrc
    std::string config_dir_path = home_dir + "/.config/slowertext/slowertextrc";
    if (FileManager::file_exists(config_dir_path)) {
        return config_dir_path;
    }
    
    // 3. ~/.slowertextrc
    std::string home_config_path = home_dir + "/.slowertextrc";
    if (FileManager::file_exists(home_config_path)) {
        return home_config_path;
    }
    
    // Fallback to runtime/slowertextrc
    return runtime_path;
}

void ConfigManager::parse_config_line(const std::string& line, std::map<std::string, std::string>& values) {
    if (line.empty() || line[0] == '#' || line[0] == '[') {
        return;
    }
    
    size_t equals_pos = line.find('=');
    if (equals_pos != std::string::npos) {
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        values[key] = value;
    }
}

void ConfigManager::apply_config_values(EditorConfig& config, const std::map<std::string, std::string>& values) {
    for (const auto& pair : values) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        
        if (key == "show_line_numbers") {
            config.show_line_numbers = string_to_bool(value);
        } else if (key == "tab_width") {
            config.tab_width = std::stoi(value);
        } else if (key == "auto_indent") {
            config.auto_indent = string_to_bool(value);
        } else if (key == "show_whitespace") {
            config.show_whitespace = string_to_bool(value);
        } else if (key == "status_format") {
            config.status_format = value;
        } else if (key == "color_scheme") {
            config.color_scheme = value;
        } else if (key == "text_color") {
            config.text_color = value;
        } else if (key == "background_color") {
            config.background_color = value;
        } else if (key == "status_bar_color") {
            config.status_bar_color = value;
        } else if (key == "comment_color") {
            config.comment_color = value;
        } else if (key == "font") {
            config.font = value;
        } else if (key == "show_tilde") {
            config.show_tilde = string_to_bool(value);
        } else if (key == "highlight_current_line") {
            config.highlight_current_line = string_to_bool(value);
        } else if (key == "confirm_quit") {
            config.confirm_quit = string_to_bool(value);
        } else if (key == "auto_save_interval") {
            config.auto_save_interval = std::stoi(value);
        } else if (key == "create_backups") {
            config.create_backups = string_to_bool(value);
        } else if (key == "max_undo_levels") {
            config.max_undo_levels = std::stoi(value);
        } else if (key == "word_wrap") {
            config.word_wrap = string_to_bool(value);
        } else if (key == "default_extension") {
            config.default_extension = value;
        } else if (key == "show_hidden_files") {
            config.show_hidden_files = string_to_bool(value);
        } else if (key == "default_encoding") {
            config.default_encoding = value;
        } else if (key == "line_endings") {
            config.line_endings = value;
        } else if (key == "buffer_size") {
            config.buffer_size = std::stoi(value);
        } else if (key == "refresh_rate") {
            config.refresh_rate = std::stoi(value);
        } else if (key == "syntax_highlighting") {
            config.syntax_highlighting = string_to_bool(value);
        } else if (key == "debug_mode") {
            config.debug_mode = string_to_bool(value);
        } else if (key == "default_mode") {
            if (value == "insert") {
                config.mode = INSERT_MODE;
            } else if (value == "command") {
                config.mode = COMMAND_MODE;
            }
        } else if (key == "enter_insert") {
            config.enter_insert = value;
        } else if (key == "enter_command") {
            config.enter_command = value;
        } else if (key == "save_file") {
            config.save_file = value;
        } else if (key == "quit_editor") {
            config.quit_editor = value;
        } else if (key == "force_quit") {
            config.force_quit = value;
        } else if (key == "cursor_up") {
            config.cursor_up = value;
        } else if (key == "cursor_down") {
            config.cursor_down = value;
        } else if (key == "cursor_left") {
            config.cursor_left = value;
        } else if (key == "cursor_right") {
            config.cursor_right = value;
        }
    }
}

bool ConfigManager::string_to_bool(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str == "true" || lower_str == "1" || lower_str == "yes" || lower_str == "on";
}

int ConfigManager::parse_key_binding(const std::string& key) {
    if (key.empty()) return 0;
    
    // Convert to lowercase for comparison
    std::string lower_key = key;
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
    
    // Handle special keys first
    if (lower_key == "escape" || lower_key == "esc") return ESC_KEY;
    if (lower_key == "backspace") return BACKSPACE_KEY;
    if (lower_key == "delete" || lower_key == "del") return DELETE_KEY;
    if (lower_key == "arrow_up" || lower_key == "up") return ARROW_UP;
    if (lower_key == "arrow_down" || lower_key == "down") return ARROW_DOWN;
    if (lower_key == "arrow_left" || lower_key == "left") return ARROW_LEFT;
    if (lower_key == "arrow_right" || lower_key == "right") return ARROW_RIGHT;
    if (lower_key == "tab") return '\t';
    if (lower_key == "enter" || lower_key == "return") return '\r';
    if (lower_key == "space") return ' ';
    
    // Handle Ctrl key combinations
    if (lower_key.substr(0, 5) == "ctrl+" && lower_key.length() == 6) {
        char ctrl_char = lower_key[5];
        if (ctrl_char >= 'a' && ctrl_char <= 'z') {
            return CTRL_KEY(ctrl_char);
        }
    }
    
    // Handle Alt key combinations (for future use)
    if (lower_key.substr(0, 4) == "alt+" && lower_key.length() == 5) {
        // Alt combinations would need more complex handling
        // For now, just return the character
        return lower_key[4];
    }
    
    // Handle function keys (for future use)
    if (lower_key.substr(0, 1) == "f" && lower_key.length() >= 2) {
        // Function keys would need special handling
        // For now, return 0
        return 0;
    }
    
    // Single character keys
    if (lower_key.length() == 1) {
        return lower_key[0];
    }
    
    // If nothing matches, return 0
    return 0;
}