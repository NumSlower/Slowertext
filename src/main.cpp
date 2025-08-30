#include "../include/slowertext.h"
#include <iostream>

// Global instances
EditorConfig editor_config;
Terminal terminal;

/**
 * Handle window resize signal (SIGWINCH)
 * Updates screen dimensions when terminal is resized
 */
void handle_sigwinch(int sig) {
    (void)sig;  // Suppress unused parameter warning
    terminal.get_window_size(&editor_config.screen_rows, &editor_config.screen_cols);
    editor_config.screen_rows -= 2; // Reserve space for status and message bars
}

/**
 * Cleanup resources and restore terminal state before exit
 */
void cleanup_and_exit() {
    terminal.clear_screen();
    terminal.set_cursor_position(0, 0);
    terminal.show_cursor();
}

/**
 * Initialize editor configuration and terminal settings
 */
void init_editor() {
    // Initialize cursor and scroll positions
    editor_config.cursor_x = 0;
    editor_config.cursor_y = 0;
    editor_config.row_offset = 0;
    editor_config.col_offset = 0;
    
    // Initialize editor state
    editor_config.mode = INSERT_MODE; // Will be overridden by config if specified
    editor_config.filename = "";
    editor_config.modified = false;
    editor_config.quit = false;
    editor_config.status_msg = "";
    editor_config.status_msg_time = 0;

    // Load configuration from RC file
    ConfigManager::load_config(editor_config);
    
    // Debug output to verify configuration loading
    if (editor_config.debug_mode) {
        std::cerr << "Debug: Configuration loaded successfully\n";
        std::cerr << "Debug: tab_width = " << editor_config.tab_width << "\n";
        std::cerr << "Debug: show_line_numbers = " << (editor_config.show_line_numbers ? "true" : "false") << "\n";
        std::cerr << "Debug: auto_indent = " << (editor_config.auto_indent ? "true" : "false") << "\n";
        std::cerr << "Debug: text_color = " << editor_config.text_color << "\n";
        std::cerr << "Debug: background_color = " << editor_config.background_color << "\n";
        std::cerr << "Debug: syntax_highlighting = " << (editor_config.syntax_highlighting ? "true" : "false") << "\n";
        std::cerr << "Press any key to continue...\n";
        getchar();
    }

    // Get terminal dimensions
    if (terminal.get_window_size(&editor_config.screen_rows, &editor_config.screen_cols) == -1) {
        std::cerr << "Error: Unable to get terminal size\n";
        exit(1);
    }
    editor_config.screen_rows -= 2; // Reserve space for status and message bars

    // Set up signal handler for window resize
    signal(SIGWINCH, handle_sigwinch);
    
    // Ensure cleanup on exit
    atexit(cleanup_and_exit);
}

/**
 * Set status message with current timestamp
 * @param msg Message to display in status bar
 */
void set_status_message(const std::string& msg) {
    editor_config.status_msg = msg;
    editor_config.status_msg_time = time(nullptr);
}

/**
 * Main application entry point
 */
int main(int argc, char* argv[]) {
    Buffer buffer;
    
    try {
        // Initialize editor
        init_editor();
        
        // Load file if specified as command line argument
        if (argc >= 2) {
            editor_config.filename = argv[1];
            if (!FileManager::load_file(editor_config.filename, buffer)) {
                set_status_message("New file: " + editor_config.filename);
            } else {
                set_status_message("Loaded: " + editor_config.filename);
            }
        } else {
            // No file specified - show welcome message with tab width info
            set_status_message("SlowerText Editor - Tab width: " + std::to_string(editor_config.tab_width) + " spaces");
        }

        // Main editor loop
        while (!editor_config.quit) {
            Renderer::refresh_screen(editor_config, buffer);
            InputHandler::process_keypress(editor_config, buffer);
        }
        
        // Clean exit
        cleanup_and_exit();
        
    } catch (const std::exception& e) {
        // Handle any exceptions and cleanup
        cleanup_and_exit();
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}