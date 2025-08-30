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
 * Load configuration from file and set defaults
 * @param config Editor configuration to populate
 */
void ConfigManager::load_config(EditorConfig& config) {
    // Set default configuration values
    config.show_line_numbers = false;
    config.tab_width = 4;
    config.auto_indent = true;
    config.show_whitespace = false;
    config.status_format = "%f%modified - %m";
    config.text_color = "white";
    config.background_color = "black";
    config.status_bar_color = "cyan";
    config.comment_color = "green";
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
    
    // Set default key bindings
    config.enter_insert = "ctrl+i";
    config.enter_command = "escape";
    config.save_file = "ctrl+s";
    config.quit_editor = "ctrl+q";
    config.force_quit = "ctrl+f";
    config.cursor_up = "arrow_up";
    config.cursor_down = "arrow_down";
    config.cursor_left = "arrow_left";
    config.cursor_right = "arrow_right";
    
    // Try to load configuration file
    std::string config_path = get_config_path();
    
    if (config.debug_mode) {
        std::cerr << "Debug: Attempting to load config from: " << config_path << std::endl;
    }
    
    std::ifstream config_file(config_path);
    
    if (!config_file.is_open()) {
        if (config.debug_mode) {
            std::cerr << "Debug: Could not open config file: " << config_path << std::endl;
            std::cerr << "Debug: Using default configuration values" << std::endl;
        }
        return; // Use defaults if config file doesn't exist
    }
    
    if (config.debug_mode) {
        std::cerr << "Debug: Successfully opened config file: " << config_path << std::endl;
    }
    
    // Parse configuration file
    std::map<std::string, std::string> config_values;
    std::string line;
    int line_number = 0;
    
    while (std::getline(config_file, line)) {
        line_number++;
        parse_config_line(line, config_values);
        if (config.debug_mode && !line.empty() && line[0] != '#') {
            std::cerr << "Debug: Parsed line " << line_number << ": " << line << std::endl;
        }
    }
    
    config_file.close();
    
    if (config.debug_mode) {
        std::cerr << "Debug: Found " << config_values.size() << " configuration values" << std::endl;
    }
    
    apply_config_values(config, config_values);
    
    if (config.debug_mode) {
        std::cerr << "Debug: Configuration applied successfully" << std::endl;
        std::cerr << "Debug: Final tab_width = " << config.tab_width << std::endl;
    }
}

/**
 * Get the path to the configuration file
 * Checks multiple locations in order of priority
 * @return Path to configuration file
 */
std::string ConfigManager::get_config_path() {
    // Get home directory
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
    
    // Check configuration file locations in order of priority:
    std::vector<std::string> config_paths;
    
    // 1. User config directory: ~/.config/slowertext/slowertextrc (highest priority)
    config_paths.push_back(home_dir + "/.config/slowertext/slowertextrc");
    
    // 2. Home directory: ~/.slowertextrc
    config_paths.push_back(home_dir + "/.slowertextrc");
    
    // 3. Local runtime/slowertextrc (for development, lowest priority)
    config_paths.push_back("runtime/slowertextrc");
    
    // 4. Global system config (if it exists)
    config_paths.push_back("/etc/slowertext/slowertextrc");
    
    // Check each path and return the first one that exists
    for (const std::string& path : config_paths) {
        if (FileManager::file_exists(path)) {
            return path;
        }
    }
    
    // If no config file exists, return the preferred user config path
    // This allows the program to know where to look, even if the file doesn't exist yet
    return home_dir + "/.config/slowertext/slowertextrc";
}

/**
 * Parse a single configuration line into key-value pairs
 * Ignores comments and empty lines
 * @param line Configuration line to parse
 * @param values Map to store parsed key-value pairs
 */
void ConfigManager::parse_config_line(const std::string& line, std::map<std::string, std::string>& values) {
    // Skip empty lines, comments, and section headers
    if (line.empty() || line[0] == '#' || line[0] == '[') {
        return;
    }
    
    // Find equals sign
    size_t equals_pos = line.find('=');
    if (equals_pos != std::string::npos) {
        std::string key = line.substr(0, equals_pos);
        std::string value = line.substr(equals_pos + 1);
        
        // Trim whitespace from key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        // Remove quotes if present
        if (value.length() >= 2 && 
            ((value.front() == '"' && value.back() == '"') || 
             (value.front() == '\'' && value.back() == '\''))) {
            value = value.substr(1, value.length() - 2);
        }
        
        if (!key.empty()) {
            values[key] = value;
        }
    }
}

/**
 * Apply parsed configuration values to editor config
 * @param config Editor configuration to update
 * @param values Map of configuration key-value pairs
 */
void ConfigManager::apply_config_values(EditorConfig& config, const std::map<std::string, std::string>& values) {
    for (const auto& pair : values) {
        const std::string& key = pair.first;
        const std::string& value = pair.second;
        
        try {
            // Display settings
            if (key == "show_line_numbers") {
                config.show_line_numbers = string_to_bool(value);
            } else if (key == "tab_width") {
                int tab_val = std::stoi(value);
                if (tab_val > 0 && tab_val <= 16) { // Reasonable range
                    config.tab_width = tab_val;
                }
            } else if (key == "auto_indent") {
                config.auto_indent = string_to_bool(value);
            } else if (key == "show_whitespace") {
                config.show_whitespace = string_to_bool(value);
            } else if (key == "status_format") {
                config.status_format = value;
            } else if (key == "text_color") {
                config.text_color = value;
            } else if (key == "background_color") {
                config.background_color = value;
            } else if (key == "status_bar_color") {
                config.status_bar_color = value;
            } else if (key == "comment_color") {
                config.comment_color = value;
            } else if (key == "show_tilde") {
                config.show_tilde = string_to_bool(value);
            } else if (key == "highlight_current_line") {
                config.highlight_current_line = string_to_bool(value);
            
            // Editor behavior settings
            } else if (key == "confirm_quit") {
                config.confirm_quit = string_to_bool(value);
            } else if (key == "auto_save_interval") {
                int interval = std::stoi(value);
                if (interval >= 0) {
                    config.auto_save_interval = interval;
                }
            } else if (key == "create_backups") {
                config.create_backups = string_to_bool(value);
            } else if (key == "max_undo_levels") {
                int levels = std::stoi(value);
                if (levels > 0) {
                    config.max_undo_levels = levels;
                }
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
                int size = std::stoi(value);
                if (size > 0) {
                    config.buffer_size = size;
                }
            } else if (key == "refresh_rate") {
                int rate = std::stoi(value);
                if (rate > 0) {
                    config.refresh_rate = rate;
                }
            } else if (key == "syntax_highlighting") {
                config.syntax_highlighting = string_to_bool(value);
            } else if (key == "debug_mode") {
                config.debug_mode = string_to_bool(value);
            
            // Default mode setting
            } else if (key == "default_mode") {
                if (value == "insert") {
                    config.mode = INSERT_MODE;
                } else if (value == "command") {
                    config.mode = COMMAND_MODE;
                }
            
            // Key bindings
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
            // Ignore unknown keys silently
        } catch (const std::exception& e) {
            // Ignore invalid values silently to prevent crashes
            // In debug mode, we could log these errors
            if (config.debug_mode) {
                std::cerr << "Debug: Error parsing config value for key '" 
                         << key << "': " << e.what() << std::endl;
            }
        }
    }
}

/**
 * Convert string to boolean value
 * Accepts various formats: true/false, 1/0, yes/no, on/off
 * @param str String to convert
 * @return Boolean value
 */
bool ConfigManager::string_to_bool(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), ::tolower);
    return lower_str == "true" || lower_str == "1" || lower_str == "yes" || lower_str == "on";
}

/**
 * Parse key binding string to key code
 * Handles special keys, control combinations, and single characters
 * @param key Key binding string (e.g., "ctrl+s", "escape", "arrow_up")
 * @return Key code integer, 0 if invalid
 */
int ConfigManager::parse_key_binding(const std::string& key) {
    if (key.empty()) return 0;
    
    // Convert to lowercase for comparison
    std::string lower_key = key;
    std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
    
    // Handle special keys
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
    
    // Handle Ctrl key combinations (ctrl+x format)
    if (lower_key.substr(0, 5) == "ctrl+" && lower_key.length() == 6) {
        char ctrl_char = lower_key[5];
        if (ctrl_char >= 'a' && ctrl_char <= 'z') {
            return CTRL_KEY(ctrl_char);
        }
    }
    
    // Handle Alt key combinations (for future expansion)
    if (lower_key.substr(0, 4) == "alt+" && lower_key.length() == 5) {
        // Alt combinations would need more complex handling
        // For now, just return the character
        return lower_key[4];
    }
    
    // Handle function keys (for future expansion)
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