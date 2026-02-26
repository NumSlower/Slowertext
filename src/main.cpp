#include "../include/slowertext.h"
#include <iostream>

EditorConfig editor_config;
Terminal     terminal;

void handle_sigwinch(int /*sig*/) {
    terminal.get_window_size(&editor_config.screen_rows, &editor_config.screen_cols);
    editor_config.screen_rows -= 2;
    editor_config.needs_redraw = true;
}

void cleanup_and_exit() {
    terminal.clear_screen();
    terminal.set_cursor_position(0, 0);
    terminal.show_cursor();
}

void set_status_message(const std::string& msg) {
    editor_config.status_msg      = msg;
    editor_config.status_msg_time = time(nullptr);
    editor_config.needs_redraw    = true;
}

static void init_editor() {
    editor_config.cursor_x          = 0;
    editor_config.cursor_y          = 0;
    editor_config.row_offset        = 0;
    editor_config.col_offset        = 0;
    editor_config.mode              = INSERT_MODE;
    editor_config.filename          = "";
    editor_config.modified          = false;
    editor_config.quit              = false;
    editor_config.status_msg        = "";
    editor_config.status_msg_time   = 0;
    editor_config.needs_redraw      = true;

    ConfigManager::load_config(editor_config);

    if (editor_config.debug_mode) {
        std::cerr << "Debug: tab_width=" << editor_config.tab_width
                  << " refresh_rate=" << editor_config.refresh_rate << '\n'
                  << "Press any key to continue...\n";
        getchar();
    }

    if (terminal.get_window_size(&editor_config.screen_rows,
                                  &editor_config.screen_cols) == -1) {
        std::cerr << "Error: Unable to get terminal size\n";
        exit(1);
    }
    editor_config.screen_rows -= 2;

    signal(SIGWINCH, handle_sigwinch);
    atexit(cleanup_and_exit);
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
                set_status_message("Loaded: " + editor_config.filename +
                                   "  |  :color <target> <value>  to change colors");
            }
        } else {
            set_status_message("SlowerText  |  :theme <dark|light|ocean|forest|neon>");
        }

        // ── Main loop ─────────────────────────────────────────────────────────
        // Order is deliberately:
        //   1. Draw if dirty  (catches the very first frame and post-keypress)
        //   2. Wait for input (select timeout = 1 frame @ refresh_rate FPS)
        //   3. Process key    (sets needs_redraw)
        //   4. Draw immediately if dirty  ← the key fix: redraw RIGHT after the
        //      keypress, before blocking on the next one, so typing feels instant
        //   5. Repeat
        while (!editor_config.quit) {

            // Always draw at the top of the loop so the very first frame renders
            // and so post-timeout dirty flags (status-msg expiry, SIGWINCH) are
            // serviced promptly.
            if (editor_config.needs_redraw) {
                Renderer::refresh_screen(editor_config, buffer);
                editor_config.needs_redraw = false;
            }

            // Block up to one frame waiting for a keypress.
            bool got_key = InputHandler::process_keypress(editor_config, buffer);

            if (got_key) {
                // Redraw immediately — don't wait for the next loop iteration.
                // This is what makes every keystroke feel instantaneous.
                if (editor_config.needs_redraw) {
                    Renderer::refresh_screen(editor_config, buffer);
                    editor_config.needs_redraw = false;
                }
            } else {
                // Timeout (no input this frame): check if the status message has
                // expired and needs to be cleared from the message bar.
                time_t now = time(nullptr);
                if (!editor_config.status_msg.empty() &&
                    (now - editor_config.status_msg_time) >= 5) {
                    editor_config.status_msg   = "";
                    editor_config.needs_redraw = true;
                }
            }
        }

        cleanup_and_exit();

    } catch (const std::exception& e) {
        cleanup_and_exit();
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}