#include "../include/slowertext.h"
#include <fstream>
#include <sstream>
#include <map>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>

void ConfigManager::load_config(EditorConfig& config) {
    // ── Defaults ──────────────────────────────────────────────────────────────
    config.show_line_numbers      = true;
    config.tab_width              = 4;
    config.auto_indent            = true;
    config.show_whitespace        = false;
    config.status_format          = "%f%modified - %m";
    config.show_tilde             = true;
    config.highlight_current_line = true;

    // Default color scheme (dark theme)
    config.text_color             = "white";
    config.background_color       = "black";
    config.status_bar_color       = "blue";
    config.status_text_color      = "white";
    config.comment_color          = "color242";
    config.line_number_color      = "color240";
    config.current_line_color     = "color236";

    config.confirm_quit           = true;
    config.auto_save_interval     = 0;
    config.create_backups         = false;
    config.max_undo_levels        = 100;
    config.word_wrap              = false;
    config.default_extension      = "txt";
    config.show_hidden_files      = false;
    config.default_encoding       = "utf-8";
    config.line_endings           = "unix";
    config.buffer_size            = 64;
    config.refresh_rate           = 30;
    config.syntax_highlighting    = true;
    config.debug_mode             = false;

    config.enter_insert   = "ctrl+j";
    config.enter_command  = "escape";
    config.save_file      = "ctrl+s";
    config.quit_editor    = "ctrl+q";
    config.force_quit     = "ctrl+f";
    config.cursor_up      = "arrow_up";
    config.cursor_down    = "arrow_down";
    config.cursor_left    = "arrow_left";
    config.cursor_right   = "arrow_right";

    // ── Parse config file ─────────────────────────────────────────────────────
    std::string config_path = get_config_path();
    std::ifstream config_file(config_path);
    std::map<std::string, std::string> vals;

    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) parse_config_line(line, vals);
        config_file.close();
    }

    apply_config_values(config, vals);

    if (config.debug_mode) {
        std::cerr << "Debug: config=" << config_path
                  << " (" << vals.size() << " keys)\n";
    }
}

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

    for (const auto& p : candidates)
        if (FileManager::file_exists(p)) return p;

    return home_dir + "/.config/slowertext/slowertextrc";
}

void ConfigManager::parse_config_line(const std::string& line,
                                       std::map<std::string,std::string>& values) {
    if (line.empty() || line[0] == '#' || line[0] == '[') return;

    size_t eq = line.find('=');
    if (eq == std::string::npos) return;

    std::string key   = line.substr(0, eq);
    std::string value = line.substr(eq + 1);

    // Trim key
    key.erase(0, key.find_first_not_of(" \t"));
    auto ke = key.find_last_not_of(" \t");
    if (ke != std::string::npos) key.erase(ke + 1);

    // Trim value leading whitespace
    value.erase(0, value.find_first_not_of(" \t"));

    // Strip inline comments
    size_t cp = value.find(" #");
    if (cp == std::string::npos) cp = value.find("\t#");
    if (cp != std::string::npos) value.erase(cp);

    // Trim value trailing whitespace
    auto ve = value.find_last_not_of(" \t");
    if (ve != std::string::npos) value.erase(ve + 1);
    else value.clear();

    // Strip surrounding quotes
    if (value.size() >= 2 &&
        ((value.front() == '"'  && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\'')))
        value = value.substr(1, value.size() - 2);

    if (!key.empty()) values[key] = value;
}

void ConfigManager::apply_config_values(EditorConfig& config,
                                         const std::map<std::string,std::string>& values) {
    for (const auto& kv : values) {
        const std::string& k = kv.first;
        const std::string& v = kv.second;
        try {
            if      (k == "show_line_numbers")      config.show_line_numbers      = string_to_bool(v);
            else if (k == "tab_width") {
                int n = std::stoi(v); if (n > 0 && n <= 16) config.tab_width = n;
            }
            else if (k == "auto_indent")            config.auto_indent            = string_to_bool(v);
            else if (k == "show_whitespace")        config.show_whitespace        = string_to_bool(v);
            else if (k == "status_format")          config.status_format          = v;
            else if (k == "show_tilde")             config.show_tilde             = string_to_bool(v);
            else if (k == "highlight_current_line") config.highlight_current_line = string_to_bool(v);
            // All color fields
            else if (k == "text_color")             config.text_color             = v;
            else if (k == "background_color")       config.background_color       = v;
            else if (k == "status_bar_color")       config.status_bar_color       = v;
            else if (k == "status_text_color")      config.status_text_color      = v;
            else if (k == "comment_color")          config.comment_color          = v;
            else if (k == "line_number_color")      config.line_number_color      = v;
            else if (k == "current_line_color")     config.current_line_color     = v;
            // Behaviour
            else if (k == "confirm_quit")           config.confirm_quit           = string_to_bool(v);
            else if (k == "auto_save_interval") {
                int n = std::stoi(v); if (n >= 0) config.auto_save_interval = n;
            }
            else if (k == "create_backups")         config.create_backups         = string_to_bool(v);
            else if (k == "max_undo_levels") {
                int n = std::stoi(v); if (n > 0) config.max_undo_levels = n;
            }
            else if (k == "word_wrap")              config.word_wrap              = string_to_bool(v);
            else if (k == "default_extension")      config.default_extension      = v;
            else if (k == "show_hidden_files")      config.show_hidden_files      = string_to_bool(v);
            else if (k == "default_encoding")       config.default_encoding       = v;
            else if (k == "line_endings")           config.line_endings           = v;
            else if (k == "buffer_size") {
                int n = std::stoi(v); if (n > 0) config.buffer_size = n;
            }
            else if (k == "refresh_rate") {
                int n = std::stoi(v); if (n > 0) config.refresh_rate = n;
            }
            else if (k == "syntax_highlighting")    config.syntax_highlighting    = string_to_bool(v);
            else if (k == "debug_mode")             config.debug_mode             = string_to_bool(v);
            else if (k == "default_mode") {
                if      (v == "insert")  config.mode = INSERT_MODE;
                else if (v == "command") config.mode = COMMAND_MODE;
            }
            // Key bindings
            else if (k == "enter_insert")   config.enter_insert   = v;
            else if (k == "enter_command")  config.enter_command  = v;
            else if (k == "save_file")      config.save_file      = v;
            else if (k == "quit_editor")    config.quit_editor    = v;
            else if (k == "force_quit")     config.force_quit     = v;
            else if (k == "cursor_up")      config.cursor_up      = v;
            else if (k == "cursor_down")    config.cursor_down    = v;
            else if (k == "cursor_left")    config.cursor_left    = v;
            else if (k == "cursor_right")   config.cursor_right   = v;
            // Silently ignore unknown keys (e.g. "font")
        } catch (const std::exception& e) {
            if (config.debug_mode)
                std::cerr << "Debug: bad config value for '" << k << "': " << e.what() << '\n';
        }
    }
}

bool ConfigManager::string_to_bool(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return (s == "true" || s == "1" || s == "yes" || s == "on");
}

int ConfigManager::parse_key_binding(const std::string& key) {
    if (key.empty()) return 0;
    std::string k = key;
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    if (k == "escape" || k == "esc")               return ESC_KEY;
    if (k == "backspace")                           return BACKSPACE_KEY;
    if (k == "delete" || k == "del")               return DELETE_KEY;
    if (k == "arrow_up"    || k == "up")           return ARROW_UP;
    if (k == "arrow_down"  || k == "down")         return ARROW_DOWN;
    if (k == "arrow_left"  || k == "left")         return ARROW_LEFT;
    if (k == "arrow_right" || k == "right")        return ARROW_RIGHT;
    if (k == "tab")                                 return '\t';
    if (k == "enter" || k == "return")             return '\r';
    if (k == "space")                               return ' ';

    if (k.size() == 6 && k.substr(0, 5) == "ctrl+") {
        char ch = k[5];
        if (ch >= 'a' && ch <= 'z') return CTRL_KEY(ch);
    }

    if (k.size() == 1) return k[0];
    return 0;
}