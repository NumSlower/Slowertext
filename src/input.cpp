#include "../include/slowertext.h"
#include <cerrno>
#include <unistd.h>
#include <iostream>

extern void set_status_message(const std::string& msg);

// ──────────────────────────────────────────────────────────────────────────────
// Key reading
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Read a single key from stdin, handling multi-byte escape sequences.
 * Returns a key code (possibly a synthetic code like ARROW_UP) or -1 on error.
 */
int InputHandler::read_key() {
    int nread;
    char c;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            set_status_message("Error: Failed to read input");
            return -1;
        }
    }

    if (c == ESC_KEY) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC_KEY;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC_KEY;

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC_KEY;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '3': return DELETE_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                }
            }
        }
        return ESC_KEY;
    }

    return static_cast<unsigned char>(c);
}

// ──────────────────────────────────────────────────────────────────────────────
// Private helper functions
// ──────────────────────────────────────────────────────────────────────────────

static void handle_cursor_movement(EditorConfig& config, Buffer& buffer, int key) {
    if (key == ARROW_UP) {
        if (config.cursor_y > 0) {
            config.cursor_y--;
            int len = static_cast<int>(buffer.get_line(config.cursor_y).size());
            if (config.cursor_x > len) config.cursor_x = len;
        }
    } else if (key == ARROW_DOWN) {
        if (config.cursor_y < buffer.get_line_count() - 1) {
            config.cursor_y++;
            int len = static_cast<int>(buffer.get_line(config.cursor_y).size());
            if (config.cursor_x > len) config.cursor_x = len;
        }
    } else if (key == ARROW_LEFT) {
        if (config.cursor_x > 0) {
            config.cursor_x--;
        } else if (config.cursor_y > 0) {
            config.cursor_y--;
            config.cursor_x = static_cast<int>(buffer.get_line(config.cursor_y).size());
        }
    } else if (key == ARROW_RIGHT) {
        std::string line = buffer.get_line(config.cursor_y);
        if (config.cursor_x < static_cast<int>(line.size())) {
            config.cursor_x++;
        } else if (config.cursor_y < buffer.get_line_count() - 1) {
            config.cursor_y++;
            config.cursor_x = 0;
        }
    }
}

static void handle_backspace(EditorConfig& config, Buffer& buffer) {
    try {
        if (config.cursor_x > 0) {
            std::string current_line = buffer.get_line(config.cursor_y);

            // Smart tab deletion: delete a full indent block of spaces if applicable
            bool deleted_tab = false;
            if (config.tab_width > 1 && config.cursor_x >= config.tab_width) {
                int start = config.cursor_x - config.tab_width;
                bool all_spaces = true;
                for (int i = start; i < config.cursor_x; i++) {
                    if (i >= static_cast<int>(current_line.size()) || current_line[i] != ' ') {
                        all_spaces = false;
                        break;
                    }
                }
                // Only delete tab-block when we're at a tab-stop boundary
                int leading = 0;
                for (int i = 0; i < start; i++) {
                    if (i < static_cast<int>(current_line.size()) && current_line[i] == ' ')
                        leading++;
                    else
                        break;
                }
                if (all_spaces && (leading % config.tab_width == 0)) {
                    for (int i = 0; i < config.tab_width; i++) {
                        buffer.delete_char(config.cursor_x - 1, config.cursor_y);
                        config.cursor_x--;
                    }
                    deleted_tab = true;
                    if (config.debug_mode) {
                        set_status_message("Deleted tab (" +
                                           std::to_string(config.tab_width) + " spaces)");
                    }
                }
            }

            if (!deleted_tab) {
                buffer.delete_char(config.cursor_x - 1, config.cursor_y);
                config.cursor_x--;
            }
        } else if (config.cursor_y > 0) {
            // Join with previous line
            std::string cur  = buffer.get_line(config.cursor_y);
            std::string prev = buffer.get_line(config.cursor_y - 1);
            config.cursor_x = static_cast<int>(prev.size());
            buffer.set_line(config.cursor_y - 1, prev + cur);
            buffer.delete_line(config.cursor_y);
            config.cursor_y--;
        }
        config.modified = buffer.is_modified();
    } catch (const std::exception& e) {
        set_status_message("Error deleting character: " + std::string(e.what()));
    }
}

static void handle_delete(EditorConfig& config, Buffer& buffer) {
    try {
        std::string line = buffer.get_line(config.cursor_y);
        if (config.cursor_x < static_cast<int>(line.size())) {
            buffer.delete_char(config.cursor_x, config.cursor_y);
        } else if (config.cursor_y < buffer.get_line_count() - 1) {
            std::string next = buffer.get_line(config.cursor_y + 1);
            buffer.set_line(config.cursor_y, line + next);
            buffer.delete_line(config.cursor_y + 1);
        }
        config.modified = buffer.is_modified();
    } catch (const std::exception& e) {
        set_status_message("Error deleting character: " + std::string(e.what()));
    }
}

static void handle_enter(EditorConfig& config, Buffer& buffer) {
    try {
        buffer.insert_newline(config.cursor_x, config.cursor_y);
        config.cursor_y++;
        config.cursor_x = 0;

        if (config.auto_indent && config.cursor_y > 0) {
            std::string prev = buffer.get_line(config.cursor_y - 1);
            int indent = 0;
            for (char ch : prev) {
                if (ch != ' ' && ch != '\t') break;
                indent++;
            }
            for (int i = 0; i < indent; i++) {
                buffer.insert_char(config.cursor_x, config.cursor_y, ' ');
                config.cursor_x++;
            }
        }
        config.modified = buffer.is_modified();
    } catch (const std::exception& e) {
        set_status_message("Error inserting newline: " + std::string(e.what()));
    }
}

static void handle_tab(EditorConfig& config, Buffer& buffer) {
    try {
        for (int i = 0; i < config.tab_width; i++) {
            buffer.insert_char(config.cursor_x, config.cursor_y, ' ');
            config.cursor_x++;
        }
        config.modified = buffer.is_modified();
        if (config.debug_mode) {
            set_status_message("Tab: inserted " + std::to_string(config.tab_width) + " spaces");
        }
    } catch (const std::exception& e) {
        set_status_message("Error inserting tab: " + std::string(e.what()));
    }
}

static void handle_save(EditorConfig& config, Buffer& buffer) {
    if (config.filename.empty()) {
        set_status_message("Error: No filename specified");
        return;
    }
    try {
        if (FileManager::save_file(config.filename, buffer)) {
            buffer.set_modified(false);
            config.modified = false;
            set_status_message("File saved: " + config.filename);
        } else {
            set_status_message("Error: Could not save file");
        }
    } catch (const std::exception& e) {
        set_status_message("Error saving file: " + std::string(e.what()));
    }
}

// BUG FIX: default argument must only appear in the declaration (header), not
// in the definition.  We use an overload here instead to keep the call sites
// working unchanged.
static void handle_quit_impl(EditorConfig& config, Buffer& buffer, bool force) {
    if (!force && config.confirm_quit && buffer.is_modified()) {
        set_status_message("File modified. Use :q! or save first");
    } else {
        config.quit = true;
    }
}

static void handle_quit(EditorConfig& config, Buffer& buffer, bool force = false) {
    handle_quit_impl(config, buffer, force);
}

static bool key_matches_binding(int key, const std::string& binding) {
    return key == ConfigManager::parse_key_binding(binding);
}

// ──────────────────────────────────────────────────────────────────────────────
// Public interface
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Process a single keypress and update editor state.
 */
void InputHandler::process_keypress(EditorConfig& config, Buffer& buffer) {
    // These are static so that a multi-key command survives across calls.
    static std::string command_buffer  = "";
    static bool        in_command_input = false;

    try {
        int c = read_key();
        if (c == -1) {
            set_status_message("Error: Invalid key input");
            return;
        }

        if (config.mode == INSERT_MODE) {
            // Switch to command mode
            if (key_matches_binding(c, config.enter_command)) {
                config.mode = COMMAND_MODE;
                set_status_message("Command mode");
                return;
            }

            if (key_matches_binding(c, config.save_file)) {
                handle_save(config, buffer);
                return;
            }
            if (key_matches_binding(c, config.quit_editor)) {
                handle_quit(config, buffer);
                return;
            }
            if (key_matches_binding(c, config.force_quit)) {
                handle_quit(config, buffer, true);
                return;
            }

            // Special editing keys (checked before generic printable-char path)
            if (c == '\t') {
                handle_tab(config, buffer);
                return;
            }
            if (c == '\r' || c == '\n') {
                handle_enter(config, buffer);
                return;
            }
            if (c == BACKSPACE_KEY || c == 127 || c == 8 || c == CTRL_KEY('h')) {
                handle_backspace(config, buffer);
                return;
            }
            if (c == DELETE_KEY) {
                handle_delete(config, buffer);
                return;
            }

            // Arrow keys
            if (c == ARROW_UP || c == ARROW_DOWN ||
                c == ARROW_LEFT || c == ARROW_RIGHT) {
                handle_cursor_movement(config, buffer, c);
                return;
            }

            // Guard: don't re-enter insert mode (and avoid Tab conflict)
            if (c != '\t' && key_matches_binding(c, config.enter_insert)) {
                set_status_message("Already in Insert mode");
                return;
            }

            // Printable ASCII
            if (c >= 32 && c <= 126) {
                try {
                    buffer.insert_char(config.cursor_x, config.cursor_y, static_cast<char>(c));
                    config.cursor_x++;
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error inserting character: " + std::string(e.what()));
                }
                return;
            }

            // Extended / UTF-8 lead bytes
            if (c > 126) {
                try {
                    buffer.insert_char(config.cursor_x, config.cursor_y, static_cast<char>(c));
                    config.cursor_x++;
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error inserting extended character: " + std::string(e.what()));
                }
                return;
            }

            if (config.debug_mode) {
                set_status_message("Unhandled control char in INSERT: " + std::to_string(c));
            }

        } else {
            // COMMAND MODE

            if (key_matches_binding(c, config.enter_insert)) {
                config.mode = INSERT_MODE;
                set_status_message("Insert mode");
                return;
            }

            if (c == ':') {
                in_command_input = true;
                command_buffer.clear();
                set_status_message(":");
                return;
            }

            if (key_matches_binding(c, config.quit_editor)) {
                handle_quit(config, buffer);
                return;
            }
            if (key_matches_binding(c, config.force_quit)) {
                handle_quit(config, buffer, true);
                return;
            }

            if (in_command_input) {
                if (c == '\r' || c == '\n') {
                    in_command_input = false;
                    try {
                        process_command(config, buffer, command_buffer);
                    } catch (const std::exception& e) {
                        set_status_message("Error processing command: " + std::string(e.what()));
                    }
                } else if ((c == BACKSPACE_KEY || c == 127 || c == 8) && !command_buffer.empty()) {
                    command_buffer.pop_back();
                    set_status_message(":" + command_buffer);
                } else if (c == ESC_KEY) {
                    in_command_input = false;
                    command_buffer.clear();
                    set_status_message("Command mode");
                } else if (c >= 32 && c <= 126) {
                    command_buffer += static_cast<char>(c);
                    set_status_message(":" + command_buffer);
                }
                return;
            }

            if (c == ARROW_UP || c == ARROW_DOWN ||
                c == ARROW_LEFT || c == ARROW_RIGHT) {
                handle_cursor_movement(config, buffer, c);
                return;
            }

            if (config.debug_mode) {
                set_status_message("Command mode key: " + std::to_string(c));
            } else {
                set_status_message("Invalid command mode key");
            }
        }

    } catch (const std::exception& e) {
        set_status_message("Critical error in key processing: " + std::string(e.what()));
    }
}

/**
 * Process a vi-style colon command.
 */
void InputHandler::process_command(EditorConfig& config, Buffer& buffer,
                                   const std::string& command) {
    try {
        if (command == "q") {
            handle_quit(config, buffer);
        } else if (command == "q!") {
            handle_quit(config, buffer, true);
        } else if (command == "s" || command == "w") {
            handle_save(config, buffer);
        } else if (command == "wq" || command == "sq") {
            if (config.filename.empty()) {
                set_status_message("Error: No filename specified");
                return;
            }
            if (FileManager::save_file(config.filename, buffer)) {
                buffer.set_modified(false);
                config.modified = false;
                config.quit = true;
            } else {
                set_status_message("Error: Could not save file");
            }
        } else if (command.size() > 6 && command.substr(0, 6) == "saves ") {
            // BUG FIX: original checked substr(0,5)=="saves" then command.length()>6
            // but then took substr(6) — length check was off by one for the space.
            // Now we check "saves " (with space) for clarity and correctness.
            std::string new_name = command.substr(6);
            if (new_name.empty()) {
                set_status_message("Error: No filename provided for save as");
                return;
            }
            if (FileManager::save_file(new_name, buffer)) {
                config.filename = new_name;
                buffer.set_modified(false);
                config.modified = false;
                set_status_message("File saved as: " + new_name);
            } else {
                set_status_message("Error: Could not save file as " + new_name);
            }
        } else {
            set_status_message("Unknown command: " + command);
        }
    } catch (const std::exception& e) {
        set_status_message("Error processing command: " + std::string(e.what()));
    }
}