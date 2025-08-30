#include "../include/slowertext.h"
#include <ctime>
#include <stdexcept>
#include <cerrno>
#include <unistd.h>
#include <iostream>

extern void set_status_message(const std::string& msg);

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
    
    // Handle Tab key explicitly
    if (c == '\t') return '\t';
    
    // Handle all other characters including extended ASCII and UTF-8
    return static_cast<unsigned char>(c);
}

void InputHandler::process_keypress(EditorConfig& config, Buffer& buffer) {
    static std::string command_buffer = "";
    static bool in_command_input = false;
    
    try {
        int c = read_key();
        if (c == -1) {
            set_status_message("Error: Invalid key input");
            return;
        }

        int enter_insert_key = ConfigManager::parse_key_binding(config.enter_insert);
        int enter_command_key = ConfigManager::parse_key_binding(config.enter_command);
        int save_file_key = ConfigManager::parse_key_binding(config.save_file);
        int quit_editor_key = ConfigManager::parse_key_binding(config.quit_editor);
        int force_quit_key = ConfigManager::parse_key_binding(config.force_quit);
        int cursor_up_key = ConfigManager::parse_key_binding(config.cursor_up);
        int cursor_down_key = ConfigManager::parse_key_binding(config.cursor_down);
        int cursor_left_key = ConfigManager::parse_key_binding(config.cursor_left);
        int cursor_right_key = ConfigManager::parse_key_binding(config.cursor_right);

        // Debug output for key bindings
        if (config.debug_mode && c == '\t') {
            set_status_message("Tab pressed (9), enter_insert=" + std::to_string(enter_insert_key) + " (" + config.enter_insert + ")");
            return;
        }

        if (config.mode == INSERT_MODE) {
            if (c == enter_insert_key && c != '\t') {
                set_status_message("Already in Insert mode");
            } else if (c == enter_command_key) {
                config.mode = COMMAND_MODE;
                set_status_message("Command mode");
            } else if (c == save_file_key) {
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
            } else if (c == quit_editor_key) {
                if (!config.confirm_quit || !buffer.is_modified()) {
                    config.quit = true;
                } else {
                    set_status_message("File modified. Use force quit or save first");
                }
            } else if (c == force_quit_key) {
                config.quit = true;
            } else if (c == '\r' || c == '\n') {
                try {
                    buffer.insert_newline(config.cursor_x, config.cursor_y);
                    config.cursor_y++;
                    config.cursor_x = 0;
                    if (config.auto_indent && config.cursor_y > 1) {
                        std::string prev_line = buffer.get_line(config.cursor_y - 1);
                        int indent = 0;
                        for (char ch : prev_line) {
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
            } else if (c == '\t') {
                try {
                    // Insert exactly tab_width spaces for Tab key
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
            } else if (c == BACKSPACE_KEY || c == 127 || c == 8 || c == CTRL_KEY('h')) {
                // Handle multiple backspace key codes: 127 (DEL), 8 (BS), Ctrl+H
                try {
                    if (config.cursor_x > 0) {
                        // Get current line for smart tab deletion
                        std::string current_line = buffer.get_line(config.cursor_y);
                        
                        // Smart tab deletion: check if we can delete a full tab width
                        bool can_delete_tab = false;
                        int spaces_to_delete = 1; // Default to single character
                        
                        // Only try smart tab deletion if tab_width > 1 and we have enough characters
                        if (config.tab_width > 1 && config.cursor_x >= config.tab_width) {
                            // Check if the previous tab_width characters are all spaces
                            // and we're at a tab boundary (cursor position is multiple of tab_width)
                            bool all_spaces = true;
                            int start_pos = config.cursor_x - config.tab_width;
                            
                            // Verify we have enough characters and they're all spaces
                            if (start_pos >= 0 && start_pos < static_cast<int>(current_line.length())) {
                                for (int i = start_pos; i < config.cursor_x; i++) {
                                    if (i >= static_cast<int>(current_line.length()) || current_line[i] != ' ') {
                                        all_spaces = false;
                                        break;
                                    }
                                }
                                
                                // Also check if we're at a tab boundary position
                                // Count spaces from start of line to see if we align with tab stops
                                int leading_spaces = 0;
                                for (int i = 0; i < start_pos; i++) {
                                    if (i < static_cast<int>(current_line.length()) && current_line[i] == ' ') {
                                        leading_spaces++;
                                    } else {
                                        break;
                                    }
                                }
                                
                                if (all_spaces && (leading_spaces % config.tab_width == 0)) {
                                    can_delete_tab = true;
                                    spaces_to_delete = config.tab_width;
                                }
                            }
                        }
                        
                        // Delete the appropriate number of characters
                        for (int i = 0; i < spaces_to_delete; i++) {
                            if (config.cursor_x > 0) {
                                buffer.delete_char(config.cursor_x - 1, config.cursor_y);
                                config.cursor_x--;
                            }
                        }
                        
                        if (config.debug_mode && can_delete_tab) {
                            set_status_message("Deleted tab (" + std::to_string(spaces_to_delete) + " spaces)");
                        }
                    } else if (config.cursor_y > 0) {
                        // Join with previous line (backspace at beginning of line)
                        std::string current_line = buffer.get_line(config.cursor_y);
                        std::string prev_line = buffer.get_line(config.cursor_y - 1);
                        
                        config.cursor_x = static_cast<int>(prev_line.length());
                        buffer.set_line(config.cursor_y - 1, prev_line + current_line);
                        buffer.delete_line(config.cursor_y);
                        config.cursor_y--;
                    }
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error deleting character: " + std::string(e.what()));
                }
            } else if (c == DELETE_KEY) {
                try {
                    std::string current_line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x < static_cast<int>(current_line.length())) {
                        buffer.delete_char(config.cursor_x, config.cursor_y);
                    } else if (config.cursor_y < buffer.get_line_count() - 1) {
                        // Join with next line
                        std::string next_line = buffer.get_line(config.cursor_y + 1);
                        buffer.set_line(config.cursor_y, current_line + next_line);
                        buffer.delete_line(config.cursor_y + 1);
                    }
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error deleting character: " + std::string(e.what()));
                }
            } else if (c == cursor_up_key) {
                if (config.cursor_y > 0) {
                    config.cursor_y--;
                    std::string line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x > static_cast<int>(line.length())) {
                        config.cursor_x = static_cast<int>(line.length());
                    }
                }
            } else if (c == cursor_down_key) {
                if (config.cursor_y < buffer.get_line_count() - 1) {
                    config.cursor_y++;
                    std::string line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x > static_cast<int>(line.length())) {
                        config.cursor_x = static_cast<int>(line.length());
                    }
                }
            } else if (c == cursor_left_key) {
                if (config.cursor_x > 0) {
                    config.cursor_x--;
                } else if (config.cursor_y > 0) {
                    config.cursor_y--;
                    config.cursor_x = static_cast<int>(buffer.get_line(config.cursor_y).length());
                }
            } else if (c == cursor_right_key) {
                std::string current_line = buffer.get_line(config.cursor_y);
                if (config.cursor_x < static_cast<int>(current_line.length())) {
                    config.cursor_x++;
                } else if (config.cursor_y < buffer.get_line_count() - 1) {
                    config.cursor_y++;
                    config.cursor_x = 0;
                }
            } else if (c >= 32 && c <= 126) {
                // Accept printable ASCII characters (32-126)
                try {
                    buffer.insert_char(config.cursor_x, config.cursor_y, static_cast<char>(c));
                    config.cursor_x++;
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error inserting character: " + std::string(e.what()));
                }
            } else if (c > 126) {
                // Handle extended ASCII/UTF-8 characters more carefully
                try {
                    buffer.insert_char(config.cursor_x, config.cursor_y, static_cast<char>(c));
                    config.cursor_x++;
                    config.modified = buffer.is_modified();
                } catch (const std::exception& e) {
                    set_status_message("Error inserting extended character: " + std::string(e.what()));
                }
            } else {
                // Debug message for control characters (0-31) that aren't handled
                if (config.debug_mode) {
                    set_status_message("Unhandled control char: " + std::to_string(c) + " (0x" + 
                        std::to_string(c) + ")");
                }
            }
        } else if (config.mode == COMMAND_MODE) {
            if (c == enter_insert_key) {
                config.mode = INSERT_MODE;
                set_status_message("Insert mode");
            } else if (c == ':') {
                in_command_input = true;
                command_buffer = "";
            } else if (c == quit_editor_key) {
                if (!config.confirm_quit || !buffer.is_modified()) {
                    config.quit = true;
                } else {
                    set_status_message("File modified. Use force quit or save first");
                }
            } else if (c == force_quit_key) {
                config.quit = true;
            } else if (in_command_input) {
                if (c == '\r' || c == '\n') {
                    in_command_input = false;
                    try {
                        process_command(config, buffer, command_buffer);
                    } catch (const std::exception& e) {
                        set_status_message("Error processing command: " + std::string(e.what()));
                    }
                } else if ((c == BACKSPACE_KEY || c == 127 || c == 8) && !command_buffer.empty()) {
                    command_buffer.pop_back();
                } else if (c >= 32 && c <= 126) {
                    // Accept printable ASCII characters for commands
                    command_buffer += static_cast<char>(c);
                }
            } else if (c == cursor_up_key || c == cursor_down_key || c == cursor_left_key || c == cursor_right_key) {
                // Temporarily switch to insert mode to handle cursor movement
                EditorMode old_mode = config.mode;
                config.mode = INSERT_MODE;
                
                // Handle the movement directly here instead of recursive call
                if (c == cursor_up_key && config.cursor_y > 0) {
                    config.cursor_y--;
                    std::string line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x > static_cast<int>(line.length())) {
                        config.cursor_x = static_cast<int>(line.length());
                    }
                } else if (c == cursor_down_key && config.cursor_y < buffer.get_line_count() - 1) {
                    config.cursor_y++;
                    std::string line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x > static_cast<int>(line.length())) {
                        config.cursor_x = static_cast<int>(line.length());
                    }
                } else if (c == cursor_left_key) {
                    if (config.cursor_x > 0) {
                        config.cursor_x--;
                    } else if (config.cursor_y > 0) {
                        config.cursor_y--;
                        config.cursor_x = static_cast<int>(buffer.get_line(config.cursor_y).length());
                    }
                } else if (c == cursor_right_key) {
                    std::string current_line = buffer.get_line(config.cursor_y);
                    if (config.cursor_x < static_cast<int>(current_line.length())) {
                        config.cursor_x++;
                    } else if (config.cursor_y < buffer.get_line_count() - 1) {
                        config.cursor_y++;
                        config.cursor_x = 0;
                    }
                }
                
                config.mode = old_mode; // Restore command mode
            } else {
                if (config.debug_mode) {
                    set_status_message("Command mode key: " + std::to_string(c));
                } else {
                    set_status_message("Invalid command mode key");
                }
            }
        }

        if (in_command_input) {
            set_status_message(":" + command_buffer);
        }
    } catch (const std::exception& e) {
        set_status_message("Critical error in key processing: " + std::string(e.what()));
    }
}

void InputHandler::process_command(EditorConfig& config, Buffer& buffer, const std::string& command) {
    try {
        if (command == "q") {
            if (!config.confirm_quit || !buffer.is_modified()) {
                config.quit = true;
            } else {
                set_status_message("File modified. Use 'q!' to force quit or save first");
            }
        } else if (command == "q!") {
            config.quit = true;
        } else if (command == "s" || command == "w") {
            if (config.filename.empty()) {
                set_status_message("Error: No filename specified");
                return;
            }
            if (FileManager::save_file(config.filename, buffer)) {
                buffer.set_modified(false);
                config.modified = false;
                set_status_message("File saved: " + config.filename);
            } else {
                set_status_message("Error: Could not save file");
            }
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
        } else if (command.substr(0, 5) == "saves" && command.length() > 6) {
            std::string filename = command.substr(6);
            if (filename.empty()) {
                set_status_message("Error: No filename provided for save as");
                return;
            }
            if (FileManager::save_file(filename, buffer)) {
                config.filename = filename;
                buffer.set_modified(false);
                config.modified = false;
                set_status_message("File saved as: " + filename);
            } else {
                set_status_message("Error: Could not save file as " + filename);
            }
        } else {
            set_status_message("Unknown command: " + command);
        }
    } catch (const std::exception& e) {
        set_status_message("Error processing command: " + std::string(e.what()));
    }
}