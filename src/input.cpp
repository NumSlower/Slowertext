#include "../include/slowertext.h"
#include <cerrno>
#include <unistd.h>
#include <sys/select.h>
#include <iostream>

extern void set_status_message(const std::string& msg);

// ─────────────────────────────────────────────────────────────────────────────
// Key reading
// ─────────────────────────────────────────────────────────────────────────────

int InputHandler::read_key() {
    char c;
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    if (nread != 1) return -1;  // no data / error

    if (c != ESC_KEY) return static_cast<unsigned char>(c);

    // Escape sequence
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC_KEY;
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC_KEY;

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC_KEY;
            if (seq[2] == '~' && seq[1] == '3') return DELETE_KEY;
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

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

static void move_cursor(EditorConfig& config, Buffer& buffer, int key) {
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
    config.needs_redraw = true;
}

static void do_backspace(EditorConfig& config, Buffer& buffer) {
    try {
        if (config.cursor_x > 0) {
            std::string line = buffer.get_line(config.cursor_y);
            // Smart-tab: delete a full indent block if applicable
            if (config.tab_width > 1 && config.cursor_x >= config.tab_width) {
                int start = config.cursor_x - config.tab_width;
                bool all_sp = true;
                for (int i = start; i < config.cursor_x; i++) {
                    if (i >= static_cast<int>(line.size()) || line[i] != ' ')
                        { all_sp = false; break; }
                }
                int leading = 0;
                for (int i = 0; i < start; i++) {
                    if (i < static_cast<int>(line.size()) && line[i] == ' ') leading++;
                    else break;
                }
                if (all_sp && (leading % config.tab_width == 0)) {
                    for (int i = 0; i < config.tab_width; i++) {
                        buffer.delete_char(config.cursor_x - 1, config.cursor_y);
                        config.cursor_x--;
                    }
                    config.modified = buffer.is_modified();
                    config.needs_redraw = true;
                    return;
                }
            }
            buffer.delete_char(config.cursor_x - 1, config.cursor_y);
            config.cursor_x--;
        } else if (config.cursor_y > 0) {
            std::string cur  = buffer.get_line(config.cursor_y);
            std::string prev = buffer.get_line(config.cursor_y - 1);
            config.cursor_x  = static_cast<int>(prev.size());
            buffer.set_line(config.cursor_y - 1, prev + cur);
            buffer.delete_line(config.cursor_y);
            config.cursor_y--;
        }
        config.modified     = buffer.is_modified();
        config.needs_redraw = true;
    } catch (const std::exception& e) {
        set_status_message("Error: " + std::string(e.what()));
    }
}

static void do_delete(EditorConfig& config, Buffer& buffer) {
    try {
        std::string line = buffer.get_line(config.cursor_y);
        if (config.cursor_x < static_cast<int>(line.size())) {
            buffer.delete_char(config.cursor_x, config.cursor_y);
        } else if (config.cursor_y < buffer.get_line_count() - 1) {
            std::string next = buffer.get_line(config.cursor_y + 1);
            buffer.set_line(config.cursor_y, line + next);
            buffer.delete_line(config.cursor_y + 1);
        }
        config.modified     = buffer.is_modified();
        config.needs_redraw = true;
    } catch (const std::exception& e) {
        set_status_message("Error: " + std::string(e.what()));
    }
}

static void do_enter(EditorConfig& config, Buffer& buffer) {
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
        config.modified     = buffer.is_modified();
        config.needs_redraw = true;
    } catch (const std::exception& e) {
        set_status_message("Error: " + std::string(e.what()));
    }
}

static void do_tab(EditorConfig& config, Buffer& buffer) {
    try {
        for (int i = 0; i < config.tab_width; i++) {
            buffer.insert_char(config.cursor_x, config.cursor_y, ' ');
            config.cursor_x++;
        }
        config.modified     = buffer.is_modified();
        config.needs_redraw = true;
    } catch (const std::exception& e) {
        set_status_message("Error: " + std::string(e.what()));
    }
}

static void do_save(EditorConfig& config, Buffer& buffer) {
    if (config.filename.empty()) {
        set_status_message("Error: No filename — use :saves <filename>");
        config.needs_redraw = true;
        return;
    }
    if (FileManager::save_file(config.filename, buffer)) {
        buffer.set_modified(false);
        config.modified = false;
        set_status_message("Saved: " + config.filename);
    } else {
        set_status_message("Error: Could not save file");
    }
    config.needs_redraw = true;
}

static void do_quit(EditorConfig& config, Buffer& buffer, bool force = false) {
    if (!force && config.confirm_quit && buffer.is_modified()) {
        set_status_message("Unsaved changes — use :q! to force quit, or :w to save");
        config.needs_redraw = true;
    } else {
        config.quit = true;
    }
}

static bool key_matches(int key, const std::string& binding) {
    return key == ConfigManager::parse_key_binding(binding);
}

// ─────────────────────────────────────────────────────────────────────────────
// process_keypress  (non-blocking: returns false if no key was ready)
// ─────────────────────────────────────────────────────────────────────────────

bool InputHandler::process_keypress(EditorConfig& config, Buffer& buffer) {
    static std::string cmd_buf       = "";
    static bool        in_cmd_input  = false;

    // ── Wait for at least one byte of input ──────────────────────────────────
    // We use select() with a timeout equal to one frame period so the main loop
    // can service timed events (status-message expiry, SIGWINCH) without
    // busy-waiting.  The timeout is the ONLY blocking point; once data arrives
    // we read and process ALL available keys before returning so that fast
    // typists never experience queued-up lag.
    {
        int fps      = (config.refresh_rate > 0) ? config.refresh_rate : 30;
        long us      = 1000000L / fps;
        struct timeval tv;
        tv.tv_sec  = 0;
        tv.tv_usec = us;
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        int ready = select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv);
        if (ready <= 0) return false;   // timeout — nothing to process this frame
    }

    // ── Process every key that is already buffered ────────────────────────────
    // After the first byte arrives we keep calling read_key() with a near-zero
    // timeout poll so we drain bursts (paste, held arrow keys) in a single pass.
    // This means the main loop redraws once for the whole burst rather than
    // once per character, which is both faster and flicker-free.
    bool processed_any = false;
    while (true) {
        // Check if more data is immediately available (0 µs timeout = pure poll)
        if (processed_any) {
            struct timeval zero = {0, 0};
            fd_set fds;
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);
            if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &zero) <= 0)
                break;  // no more data right now — done with this burst
        }

        try {
            int c = read_key();
            if (c == -1) break;
            processed_any = true;

            if (config.mode == INSERT_MODE) {
                // ── Insert mode ──────────────────────────────────────────────
                if (key_matches(c, config.enter_command)) {
                    config.mode = COMMAND_MODE;
                    set_status_message("-- COMMAND --");
                    config.needs_redraw = true;
                    break; // mode switch — redraw before waiting for next key
                }
                if (key_matches(c, config.save_file))   { do_save(config, buffer);        continue; }
                if (key_matches(c, config.quit_editor)) { do_quit(config, buffer);         break;    }
                if (key_matches(c, config.force_quit))  { do_quit(config, buffer, true);   break;    }

                if (c == '\t')                           { do_tab(config, buffer);          continue; }
                if (c == '\r' || c == '\n')              { do_enter(config, buffer);        continue; }
                if (c == BACKSPACE_KEY || c == 8 || c == CTRL_KEY('h'))
                                                         { do_backspace(config, buffer);    continue; }
                if (c == DELETE_KEY)                     { do_delete(config, buffer);       continue; }

                if (c == ARROW_UP || c == ARROW_DOWN ||
                    c == ARROW_LEFT || c == ARROW_RIGHT) { move_cursor(config, buffer, c); continue; }

                if (c != '\t' && key_matches(c, config.enter_insert)) {
                    set_status_message("-- INSERT --");
                    config.needs_redraw = true;
                    continue;
                }

                if ((c >= 32 && c <= 126) || c > 126) {
                    buffer.insert_char(config.cursor_x, config.cursor_y, static_cast<char>(c));
                    config.cursor_x++;
                    config.modified     = buffer.is_modified();
                    config.needs_redraw = true;
                    continue;
                }

            } else {
                // ── Command mode ─────────────────────────────────────────────
                if (key_matches(c, config.enter_insert)) {
                    config.mode = INSERT_MODE;
                    set_status_message("-- INSERT --");
                    config.needs_redraw = true;
                    break; // mode switch — redraw immediately
                }

                if (c == ':') {
                    in_cmd_input = true;
                    cmd_buf.clear();
                    set_status_message(":");
                    config.needs_redraw = true;
                    break; // show the prompt immediately
                }

                if (key_matches(c, config.quit_editor)) { do_quit(config, buffer);       break; }
                if (key_matches(c, config.force_quit))  { do_quit(config, buffer, true); break; }
                if (key_matches(c, config.save_file))   { do_save(config, buffer);       break; }

                if (in_cmd_input) {
                    if (c == '\r' || c == '\n') {
                        in_cmd_input = false;
                        try { process_command(config, buffer, cmd_buf); }
                        catch (const std::exception& e) {
                            set_status_message("Command error: " + std::string(e.what()));
                        }
                    } else if ((c == BACKSPACE_KEY || c == 8) && !cmd_buf.empty()) {
                        cmd_buf.pop_back();
                        set_status_message(":" + cmd_buf);
                    } else if (c == ESC_KEY) {
                        in_cmd_input = false;
                        cmd_buf.clear();
                        set_status_message("-- COMMAND --");
                    } else if (c >= 32 && c <= 126) {
                        cmd_buf += static_cast<char>(c);
                        set_status_message(":" + cmd_buf);
                    }
                    config.needs_redraw = true;
                    break; // redraw the command line after every character
                }

                if (c == ARROW_UP || c == ARROW_DOWN ||
                    c == ARROW_LEFT || c == ARROW_RIGHT) { move_cursor(config, buffer, c); continue; }

                if (config.debug_mode)
                    set_status_message("Key: " + std::to_string(c));
                else
                    set_status_message("Unknown key — press : for commands, Ctrl+J for insert");
                config.needs_redraw = true;
            }

        } catch (const std::exception& e) {
            set_status_message("Critical error: " + std::string(e.what()));
            config.needs_redraw = true;
            break;
        }
    } // end burst-drain while loop

    return processed_any;
}

// ─────────────────────────────────────────────────────────────────────────────
// process_command
// ─────────────────────────────────────────────────────────────────────────────

void InputHandler::process_command(EditorConfig& config, Buffer& buffer,
                                    const std::string& cmd) {
    config.needs_redraw = true;

    // ── Quit ─────────────────────────────────────────────────────────────────
    if (cmd == "q")             { do_quit(config, buffer);       return; }
    if (cmd == "q!")            { do_quit(config, buffer, true); return; }

    // ── Save ─────────────────────────────────────────────────────────────────
    if (cmd == "w" || cmd == "s") { do_save(config, buffer); return; }

    if (cmd == "wq" || cmd == "sq") {
        do_save(config, buffer);
        if (!buffer.is_modified()) config.quit = true;
        return;
    }

    // ── Save-as  :saves <filename> ───────────────────────────────────────────
    if (cmd.size() > 6 && cmd.substr(0, 6) == "saves ") {
        std::string newname = cmd.substr(6);
        if (newname.empty()) { set_status_message("Error: no filename"); return; }
        if (FileManager::save_file(newname, buffer)) {
            config.filename = newname;
            buffer.set_modified(false);
            config.modified = false;
            set_status_message("Saved as: " + newname);
        } else {
            set_status_message("Error: could not save as " + newname);
        }
        return;
    }

    // ── Color command  :color <target> <spec> ────────────────────────────────
    // target: text | bg | background | statusbar | statustext |
    //         comment | linenumber | currentline
    // spec:   named | colorN | #rrggbb
    //
    // Examples:
    //   :color text #00ff88
    //   :color bg color235
    //   :color statusbar magenta
    //   :color currentline #1a1a2e
    if (cmd.size() > 6 && cmd.substr(0, 6) == "color ") {
        std::string rest   = cmd.substr(6);
        size_t sp          = rest.find(' ');
        if (sp == std::string::npos) {
            set_status_message("Usage: :color <target> <value>  — try :color text #rrggbb");
            return;
        }
        std::string target_str = rest.substr(0, sp);
        std::string value      = rest.substr(sp + 1);

        // Quick validation: make sure we can produce a non-empty escape for it
        if (resolve_fg(value).empty() && resolve_bg(value).empty()) {
            set_status_message("Unknown color: '" + value +
                               "'  — use name/colorN/#rrggbb");
            return;
        }

        ColorTarget t = parse_color_target(target_str);
        switch (t) {
            case COLOR_TARGET_TEXT:
                config.text_color = value;
                set_status_message("text color → " + value);
                break;
            case COLOR_TARGET_BACKGROUND:
                config.background_color = value;
                set_status_message("background color → " + value);
                break;
            case COLOR_TARGET_STATUS_BAR:
                config.status_bar_color = value;
                set_status_message("status bar color → " + value);
                break;
            case COLOR_TARGET_STATUS_TEXT:
                config.status_text_color = value;
                set_status_message("status text color → " + value);
                break;
            case COLOR_TARGET_COMMENT:
                config.comment_color = value;
                set_status_message("comment color → " + value);
                break;
            case COLOR_TARGET_LINE_NUMBER:
                config.line_number_color = value;
                set_status_message("line number color → " + value);
                break;
            case COLOR_TARGET_CURRENT_LINE:
                config.current_line_color = value;
                set_status_message("current line color → " + value);
                break;
            default:
                set_status_message("Unknown target '" + target_str +
                    "'.  Targets: text bg statusbar statustext comment linenumber currentline");
                return;
        }
        return;
    }

    // ── theme <name>  — built-in preset themes ────────────────────────────────
    if (cmd.size() > 6 && cmd.substr(0, 6) == "theme ") {
        std::string theme = cmd.substr(6);
        if (theme == "dark") {
            config.text_color         = "white";
            config.background_color   = "black";
            config.status_bar_color   = "blue";
            config.status_text_color  = "white";
            config.comment_color      = "color242";
            config.line_number_color  = "color240";
            config.current_line_color = "color236";
            set_status_message("Theme: dark");
        } else if (theme == "light") {
            config.text_color         = "black";
            config.background_color   = "white";
            config.status_bar_color   = "color252";
            config.status_text_color  = "black";
            config.comment_color      = "color244";
            config.line_number_color  = "color248";
            config.current_line_color = "color254";
            set_status_message("Theme: light");
        } else if (theme == "ocean") {
            config.text_color         = "#cdd6f4";
            config.background_color   = "#1e1e2e";
            config.status_bar_color   = "#313244";
            config.status_text_color  = "#89b4fa";
            config.comment_color      = "#6c7086";
            config.line_number_color  = "#45475a";
            config.current_line_color = "#313244";
            set_status_message("Theme: ocean");
        } else if (theme == "forest") {
            config.text_color         = "#d4be98";
            config.background_color   = "#282828";
            config.status_bar_color   = "#3c3836";
            config.status_text_color  = "#a9b665";
            config.comment_color      = "#928374";
            config.line_number_color  = "#504945";
            config.current_line_color = "#32302f";
            set_status_message("Theme: forest");
        } else if (theme == "neon") {
            config.text_color         = "#f8f8f2";
            config.background_color   = "#0d0d0d";
            config.status_bar_color   = "#1a1a2e";
            config.status_text_color  = "#ff79c6";
            config.comment_color      = "#6272a4";
            config.line_number_color  = "#44475a";
            config.current_line_color = "#16213e";
            set_status_message("Theme: neon");
        } else {
            set_status_message("Unknown theme '" + theme +
                "'.  Themes: dark light ocean forest neon");
            return;
        }
        return;
    }

    // ── colors  (show current palette) ───────────────────────────────────────
    if (cmd == "colors") {
        set_status_message(
            "text=" + config.text_color +
            " bg=" + config.background_color +
            " bar=" + config.status_bar_color);
        return;
    }

    set_status_message("Unknown command: " + cmd +
                       "  (try :color, :theme, :w, :q, :saves)");
}