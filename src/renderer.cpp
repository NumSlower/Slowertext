#include "../include/slowertext.h"
#include <cstring>
#include <ctime>

/**
 * Convert a color name to the corresponding ANSI foreground escape code.
 * Returns an empty string for unknown names (safe to write — writes 0 bytes).
 */
static std::string get_color_code(const std::string& color) {
    if (color == "black")   return COLOR_BLACK;
    if (color == "red")     return COLOR_RED;
    if (color == "green")   return COLOR_GREEN;
    if (color == "yellow")  return COLOR_YELLOW;
    if (color == "blue")    return COLOR_BLUE;
    if (color == "magenta") return COLOR_MAGENTA;
    if (color == "cyan")    return COLOR_CYAN;
    if (color == "white")   return COLOR_WHITE;
    // Background variants
    if (color == "bg_black")   return BG_BLACK;
    if (color == "bg_red")     return BG_RED;
    if (color == "bg_green")   return BG_GREEN;
    if (color == "bg_yellow")  return BG_YELLOW;
    if (color == "bg_blue")    return BG_BLUE;
    if (color == "bg_magenta") return BG_MAGENTA;
    if (color == "bg_cyan")    return BG_CYAN;
    if (color == "bg_white")   return BG_WHITE;
    return "";
}

/**
 * Helper: write a std::string to STDOUT.
 */
static void write_str(const std::string& s) {
    if (!s.empty()) {
        twrite(STDOUT_FILENO, s.c_str(), s.size());
    }
}

/**
 * Draw text rows on the screen.
 * Handles line numbers, syntax highlighting, and current-line highlighting.
 */
void Renderer::draw_rows(const EditorConfig& config, const Buffer& buffer) {
    std::string text_color    = get_color_code(config.text_color);
    std::string bg_color      = get_color_code("bg_" + config.background_color);
    std::string comment_color = get_color_code(config.comment_color);

    for (int y = 0; y < config.screen_rows; y++) {
        int file_row = y + config.row_offset;

        // Highlight current line or apply background color
        if (config.highlight_current_line && file_row == config.cursor_y) {
            twrite(STDOUT_FILENO, "\x1b[7m", 4); // reverse video
        } else {
            write_str(bg_color);
        }

        // Line numbers
        if (config.show_line_numbers) {
            char line_num[16];
            if (file_row < buffer.get_line_count()) {
                snprintf(line_num, sizeof(line_num), "%4d ", file_row + 1);
            } else {
                snprintf(line_num, sizeof(line_num), "     ");
            }
            twrite(STDOUT_FILENO, line_num, strlen(line_num));
        }

        // Line content
        if (file_row >= buffer.get_line_count()) {
            // Beyond end-of-file
            if (config.show_tilde) {
                write_str(text_color);
                twrite(STDOUT_FILENO, "~", 1);
            }
        } else {
            std::string line = buffer.get_line(file_row);
            int line_len = static_cast<int>(line.size());

            // How many columns are visible after horizontal scroll?
            int visible_start = config.col_offset;
            int visible_len   = line_len - visible_start;
            if (visible_len < 0) visible_len = 0;
            if (visible_len > config.screen_cols) visible_len = config.screen_cols;

            if (visible_len > 0) {
                // Basic syntax highlighting: whole-line comments
                bool is_comment = config.syntax_highlighting &&
                                  (line.find('#')  == 0 ||
                                   line.find("//") == 0);
                write_str(is_comment ? comment_color : text_color);
                twrite(STDOUT_FILENO,
                      line.c_str() + visible_start,
                      static_cast<size_t>(visible_len));
            }

            // Tilde on genuinely empty lines (not just lines that scroll off)
            if (line.empty() && config.show_tilde) {
                write_str(text_color);
                twrite(STDOUT_FILENO, "~", 1);
            }
        }

        // Reset colours, erase to end-of-line, newline
        twrite(STDOUT_FILENO, COLOR_RESET, COLOR_RESET_LEN);
        twrite(STDOUT_FILENO, CLEAR_LINE,  CLEAR_LINE_LEN);
        twrite(STDOUT_FILENO, "\r\n", 2);
    }
}

/**
 * Draw status bar showing file info and editor mode.
 */
void Renderer::draw_status_bar(const EditorConfig& config, const Buffer& buffer) {
    std::string status_color = get_color_code("bg_" + config.status_bar_color);
    write_str(status_color);

    std::string mode_str  = (config.mode == INSERT_MODE) ? "INSERT" : "COMMAND";
    std::string filename  = config.filename.empty() ? "[No Name]" : config.filename;
    std::string modified  = config.modified ? "*" : "";

    // Expand format placeholders
    std::string fmt = config.status_format;
    auto replace_all = [&](const std::string& token, const std::string& value) {
        size_t pos = 0;
        while ((pos = fmt.find(token, pos)) != std::string::npos) {
            fmt.replace(pos, token.size(), value);
            pos += value.size();
        }
    };
    replace_all("%f",        filename);
    replace_all("%modified", modified);
    replace_all("%m",        mode_str);

    // Left side — truncate to screen width
    int len = static_cast<int>(fmt.size());
    if (len > config.screen_cols) len = config.screen_cols;
    twrite(STDOUT_FILENO, fmt.c_str(), static_cast<size_t>(len));

    // Right side — cursor position (right-aligned)
    char rstatus[32];
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
                        config.cursor_y + 1, buffer.get_line_count());

    while (len < config.screen_cols) {
        if (config.screen_cols - len == rlen) {
            twrite(STDOUT_FILENO, rstatus, static_cast<size_t>(rlen));
            break;
        }
        twrite(STDOUT_FILENO, " ", 1);
        len++;
    }

    twrite(STDOUT_FILENO, COLOR_RESET, COLOR_RESET_LEN);
    twrite(STDOUT_FILENO, "\r\n", 2);
}

/**
 * Draw message bar at the bottom of the screen.
 * Messages are shown for up to 5 seconds.
 */
void Renderer::draw_message_bar(const EditorConfig& config) {
    twrite(STDOUT_FILENO, CLEAR_LINE, CLEAR_LINE_LEN);

    int msglen = static_cast<int>(config.status_msg.size());
    if (msglen > config.screen_cols) msglen = config.screen_cols;

    if (msglen > 0 && (time(nullptr) - config.status_msg_time) < 5) {
        twrite(STDOUT_FILENO, config.status_msg.c_str(), static_cast<size_t>(msglen));
    }
}

/**
 * Refresh the entire screen.
 */
void Renderer::refresh_screen(const EditorConfig& config, const Buffer& buffer) {
    // Work on a local copy so scroll() can update offsets without a const-cast
    EditorConfig mcfg = config;
    scroll(mcfg, buffer);

    terminal.hide_cursor();
    terminal.set_cursor_position(0, 0);

    write_str(get_color_code("bg_" + config.background_color));

    draw_rows(mcfg, buffer);
    draw_status_bar(mcfg, buffer);
    draw_message_bar(mcfg);

    // Compute on-screen cursor position
    int line_number_offset = mcfg.show_line_numbers ? 5 : 0;
    int screen_x = (mcfg.cursor_x - mcfg.col_offset) + line_number_offset;
    int screen_y =  mcfg.cursor_y - mcfg.row_offset;

    terminal.set_cursor_position(screen_x, screen_y);
    terminal.show_cursor();

    // Propagate updated scroll offsets back to the global config
    editor_config.row_offset = mcfg.row_offset;
    editor_config.col_offset = mcfg.col_offset;
}

/**
 * Adjust scroll offsets to keep the cursor visible.
 */
void Renderer::scroll(EditorConfig& config, const Buffer& buffer) {
    // Clamp cursor_y to valid buffer range
    int line_count = buffer.get_line_count();
    if (config.cursor_y >= line_count) config.cursor_y = line_count - 1;
    if (config.cursor_y < 0)           config.cursor_y = 0;

    // Clamp cursor_x to current line length
    int line_len = static_cast<int>(buffer.get_line(config.cursor_y).size());
    if (config.cursor_x > line_len) config.cursor_x = line_len;
    if (config.cursor_x < 0)        config.cursor_x = 0;

    // Vertical scroll
    if (config.cursor_y < config.row_offset) {
        config.row_offset = config.cursor_y;
    }
    if (config.cursor_y >= config.row_offset + config.screen_rows) {
        config.row_offset = config.cursor_y - config.screen_rows + 1;
    }

    // Horizontal scroll
    if (config.cursor_x < config.col_offset) {
        config.col_offset = config.cursor_x;
    }
    if (config.cursor_x >= config.col_offset + config.screen_cols) {
        config.col_offset = config.cursor_x - config.screen_cols + 1;
    }
}