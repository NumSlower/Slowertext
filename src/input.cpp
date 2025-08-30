#include "../include/slowertext.h"
#include <cerrno>
#include <unistd.h>
#include <iostream>

// Forward declaration for status message function
extern void set_status_message(const std::string& msg);

/**
 * Read a single key from input with escape sequence handling
 * @return Key code or -1 on error
 */
int InputHandler::read_key() {
    int nread;
    char c;
    
    // Read single character with error handling
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            set_status_message("Error: Failed to read input");
            return -1;
        }
    }

    // Handle escape sequences for special keys
    if (c == ESC_KEY) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC_KEY;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC_KEY;

        if (seq[0] == '[') {
            // Handle numbered escape sequences
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC_KEY;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '3': return DELETE_KEY;
                    }
                }
            } else {
                // Handle arrow keys
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
    
    // Return character as unsigned to handle extended ASCII
    return static_cast<unsigned char>(c);
}

/**
 * Handle cursor movement in both modes
 * @param config Editor configuration
 * @param buffer Text buffer
 * @param key Key code for movement
 */
void handle_cursor_movement(EditorConfig& config, Buffer& buffer, int key) {
    if (key == ARROW_UP) {
        if (config.cursor_y > 0) {
            config.cursor_y--;
            // Adjust cursor x to fit within new line length
            std::string line = buffer.get_line(config.cursor_y);
            if (config.cursor_x > static_cast<int>(line.length())) {
                config.cursor_x = static_cast<int>(line.length());
            }
        }
    } else if (key == ARROW_DOWN) {
        if (config.cursor_y < buffer.get_line_count() - 1) {
            config.cursor_y++;
            // Adjust cursor x to fit within new line length
            std::string line = buffer.get_line(config.cursor_y);
            if (config.cursor_x > static_cast<int>(line.length())) {
                config.cursor_x = static_cast<int>(line.length());
            }
        }
    } else if (key == ARROW_LEFT) {
        if (config.cursor_x > 0) {
            config.cursor_x--;
        } else if (config.cursor_y > 0) {
            // Move to end of previous line
            config.cursor_y--;
            config.cursor_x = static_cast<int>(buffer.get_line(config.cursor_y).length());
        }
    } else if (key == ARROW_RIGHT) {
        std::string current_line = buffer.get_line(config.cursor_y);
        if (config.cursor_x < static_cast<int>(current_line.length())) {
            config.cursor_x++;
        } else if (config.cursor_y < buffer.get_line_count() - 1) {
            // Move to beginning of next line
            config.cursor_y++;
            config.cursor_x = 0;
        }
    }
}

/**
 * Handle backspace with smart tab deletion
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void handle_backspace(EditorConfig& config, Buffer& buffer) {
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
                    
                    // Check if we're at a tab boundary position
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
}

/**
 * Handle delete key
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void handle_delete(EditorConfig& config, Buffer& buffer) {
    try {
        std::string current_line = buffer.get_line(config.cursor_y);
        if (config.cursor_x < static_cast<int>(current_line.length())) {
            // Delete character at cursor position
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
}

/**
 * Handle enter key (newline insertion)
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void handle_enter(EditorConfig& config, Buffer& buffer) {
    try {
        buffer.insert_newline(config.cursor_x, config.cursor_y);
        config.cursor_y++;
        config.cursor_x = 0;
        
        // Auto-indent if enabled
        if (config.auto_indent && config.cursor_y > 0) {
            std::string prev_line = buffer.get_line(config.cursor_y - 1);
            int indent = 0;
            for (char ch : prev_line) {
                if (ch != ' ' && ch != '\t') break;
                indent++;
            }
            // Insert indentation spaces
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

/**
 * Handle tab key insertion
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void handle_tab(EditorConfig& config, Buffer& buffer) {
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
}

/**
 * Handle file save operation
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void handle_save(EditorConfig& config, Buffer& buffer) {
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

/**
 * Handle quit operation with confirmation if needed
 * @param config Editor configuration
 * @param buffer Text buffer
 * @param force Whether to force quit without confirmation
 */
void handle_quit(EditorConfig& config, Buffer& buffer, bool force = false) {
    if (!force && config.confirm_quit && buffer.is_modified()) {
        set_status_message("File modified. Use force quit or save first");
    } else {
        config.quit = true;
    }
}

/**
 * Check if a key matches a configured key binding
 * @param key The pressed key
 * @param binding_key The configured key binding
 * @return True if they match
 */
bool key_matches_binding(int key, const std::string& binding_key) {
    int parsed_key = ConfigManager::parse_key_binding(binding_key);
    return key == parsed_key;
}

/**
 * Process keypress and update editor state
 * Main input processing function that delegates to specific handlers
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void InputHandler::process_keypress(EditorConfig& config, Buffer& buffer) {
    static std::string command_buffer = "";
    static bool in_command_input = false;
    
    try {
        int c = read_key();
        if (c == -1) {
            set_status_message("Error: Invalid key input");
            return;
        }

        // Handle keys based on current mode
        if (config.mode == INSERT_MODE) {
            // INSERT MODE HANDLING
            
            // Check for mode switching to command mode first
            if (key_matches_binding(c, config.enter_command)) {
                config.mode = COMMAND_MODE;
                set_status_message("Command mode");
                return;
            }
            
            // Check for save/quit operations
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
            
            // Handle special keys with higher priority than key bindings
            if (c == '\t') {
                // Tab key always inserts spaces in insert mode
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
            
            // Handle arrow keys
            if (c == ARROW_UP || c == ARROW_DOWN || c == ARROW_LEFT || c == ARROW_RIGHT) {
                handle_cursor_movement(config, buffer, c);
                return;
            }
            
            // Check for insert mode key binding (but not if it conflicts with tab)
            if (key_matches_binding(c, config.enter_insert) && c != '\t') {
                set_status_message("Already in Insert mode");
                return;
            }
            
            // Handle printable ASCII characters
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
            
            // Handle extended ASCII/UTF-8 characters
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
            
            // Debug message for unhandled control characters
            if (config.debug_mode) {
                set_status_message("Unhandled control char in INSERT: " + std::to_string(c) + 
                                 " (0x" + std::to_string(c) + ")");
            }
            
        } else if (config.mode == COMMAND_MODE) {
            // COMMAND MODE HANDLING
            
            if (key_matches_binding(c, config.enter_insert)) {
                config.mode = INSERT_MODE;
                set_status_message("Insert mode");
                return;
            }
            
            if (c == ':') {
                in_command_input = true;
                command_buffer = "";
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
                // Handle command input
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
            } else if (c == ARROW_UP || c == ARROW_DOWN || c == ARROW_LEFT || c == ARROW_RIGHT) {
                // Allow cursor movement in command mode
                handle_cursor_movement(config, buffer, c);
            } else {
                if (config.debug_mode) {
                    set_status_message("Command mode key: " + std::to_string(c));
                } else {
                    set_status_message("Invalid command mode key");
                }
            }
        }

        // Update status message for command input
        if (in_command_input) {
            set_status_message(":" + command_buffer);
        }
        
    } catch (const std::exception& e) {
        set_status_message("Critical error in key processing: " + std::string(e.what()));
    }
}

/**
 * Process command mode commands (vi-like commands)
 * @param config Editor configuration
 * @param buffer Text buffer
 * @param command Command string to process
 */
void InputHandler::process_command(EditorConfig& config, Buffer& buffer, const std::string& command) {
    try {
        if (command == "q") {
            // Quit command
            handle_quit(config, buffer);
        } else if (command == "q!") {
            // Force quit command
            handle_quit(config, buffer, true);
        } else if (command == "s" || command == "w") {
            // Save command
            handle_save(config, buffer);
        } else if (command == "wq" || command == "sq") {
            // Save and quit command
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
            // Save as command
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