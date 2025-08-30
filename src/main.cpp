#include "../include/slowertext.h"
#include <ctime>
#include <iostream>

EditorConfig editor_config;
Terminal terminal;

void handle_sigwinch(int sig) {
    (void)sig;
    terminal.get_window_size(&editor_config.screen_rows, &editor_config.screen_cols);
    editor_config.screen_rows -= 2; // Reserve space for status and message bars
}

void cleanup_and_exit() {
    terminal.clear_screen();
    terminal.set_cursor_position(0, 0);
    terminal.show_cursor();
}

void init_editor() {
    editor_config.cursor_x = 0;
    editor_config.cursor_y = 0;
    editor_config.row_offset = 0;
    editor_config.col_offset = 0;
    editor_config.mode = INSERT_MODE; // Default, will be overridden by config
    editor_config.filename = "";
    editor_config.modified = false;
    editor_config.quit = false;
    editor_config.status_msg = "";
    editor_config.status_msg_time = 0;

    // Load configuration from RC file
    ConfigManager::load_config(editor_config);
    
    // Debug output to verify configuration loading
    if (editor_config.debug_mode) {
        std::cerr << "Debug: Configuration loaded successfully" << std::endl;
        std::cerr << "Debug: tab_width = " << editor_config.tab_width << std::endl;
        std::cerr << "Debug: show_line_numbers = " << (editor_config.show_line_numbers ? "true" : "false") << std::endl;
        std::cerr << "Debug: auto_indent = " << (editor_config.auto_indent ? "true" : "false") << std::endl;
        std::cerr << "Debug: text_color = " << editor_config.text_color << std::endl;
        std::cerr << "Debug: background_color = " << editor_config.background_color << std::endl;
        std::cerr << "Debug: syntax_highlighting = " << (editor_config.syntax_highlighting ? "true" : "false") << std::endl;
        std::cerr << "Press any key to continue..." << std::endl;
        getchar();
    }

    if (terminal.get_window_size(&editor_config.screen_rows, &editor_config.screen_cols) == -1) {
        std::cerr << "Error: Unable to get terminal size" << std::endl;
        exit(1);
    }
    editor_config.screen_rows -= 2; // Reserve space for status and message bars

    signal(SIGWINCH, handle_sigwinch);
    atexit(cleanup_and_exit); // Ensure cleanup on exit
}

void set_status_message(const std::string& msg) {
    editor_config.status_msg = msg;
    editor_config.status_msg_time = time(nullptr);
}

int main(int argc, char* argv[]) {
    Buffer buffer;
    
    try {
        init_editor();
        
        if (argc >= 2) {
            editor_config.filename = argv[1];
            if (!FileManager::load_file(editor_config.filename, buffer)) {
                set_status_message("New file: " + editor_config.filename);
            } else {
                set_status_message("Loaded: " + editor_config.filename);
            }
        } else {
            set_status_message("SlowerText Editor - Tab width: " + std::to_string(editor_config.tab_width) + " spaces");
        }

        while (!editor_config.quit) {
            Renderer::refresh_screen(editor_config, buffer);
            InputHandler::process_keypress(editor_config, buffer);
        }
        
        // Clean exit
        cleanup_and_exit();
        
    } catch (const std::exception& e) {
        cleanup_and_exit();
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}