#include "../include/slowertext.h"
#include <cstring>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cctype>

// ─────────────────────────────────────────────────────────────────────────────
// Color resolution
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Map the 8 legacy color names → their ANSI index (0-7).
 * Returns -1 for unknown names.
 */
static int named_color_index(const std::string& s) {
    if (s == "black")   return 0;
    if (s == "red")     return 1;
    if (s == "green")   return 2;
    if (s == "yellow")  return 3;
    if (s == "blue")    return 4;
    if (s == "magenta") return 5;
    if (s == "cyan")    return 6;
    if (s == "white")   return 7;
    return -1;
}

/**
 * Resolve a color specification to an ANSI SGR foreground escape string.
 *
 * Accepted formats:
 *   "black" | "red" | "green" | "yellow" | "blue" | "magenta" | "cyan" | "white"
 *   "bright_black" | "bright_red" | … (adds +60 to the ANSI code)
 *   "colorN"   where N is 0-255   → ESC[38;5;Nm
 *   "#rrggbb"  24-bit hex         → ESC[38;2;R;G;Bm
 *   ""         (empty)            → "" (no-op)
 */
std::string resolve_fg(const std::string& spec) {
    if (spec.empty()) return "";

    // bright_ prefix
    if (spec.size() > 7 && spec.substr(0, 7) == "bright_") {
        int idx = named_color_index(spec.substr(7));
        if (idx >= 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "\033[%dm", 90 + idx);
            return buf;
        }
    }

    // Named 8-color
    int idx = named_color_index(spec);
    if (idx >= 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "\033[%dm", 30 + idx);
        return buf;
    }

    // colorN  (256-color palette)
    if (spec.size() > 5 && spec.substr(0, 5) == "color") {
        bool ok = true;
        for (size_t i = 5; i < spec.size(); i++)
            if (!isdigit(static_cast<unsigned char>(spec[i]))) { ok = false; break; }
        if (ok) {
            int n = std::stoi(spec.substr(5));
            if (n >= 0 && n <= 255) {
                char buf[24];
                snprintf(buf, sizeof(buf), "\033[38;5;%dm", n);
                return buf;
            }
        }
    }

    // #rrggbb  (24-bit / true color)
    if (spec.size() == 7 && spec[0] == '#') {
        bool hex_ok = true;
        for (int i = 1; i <= 6; i++)
            if (!isxdigit(static_cast<unsigned char>(spec[i]))) { hex_ok = false; break; }
        if (hex_ok) {
            unsigned int r = 0, g = 0, b = 0;
            sscanf(spec.c_str() + 1, "%2x%2x%2x", &r, &g, &b);
            char buf[32];
            snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", r, g, b);
            return buf;
        }
    }

    return ""; // unknown — caller should fall back
}

/**
 * Resolve a color specification to an ANSI SGR background escape string.
 * Same format as resolve_fg().
 */
std::string resolve_bg(const std::string& spec) {
    if (spec.empty()) return "";

    if (spec.size() > 7 && spec.substr(0, 7) == "bright_") {
        int idx = named_color_index(spec.substr(7));
        if (idx >= 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "\033[%dm", 100 + idx);
            return buf;
        }
    }

    int idx = named_color_index(spec);
    if (idx >= 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "\033[%dm", 40 + idx);
        return buf;
    }

    if (spec.size() > 5 && spec.substr(0, 5) == "color") {
        bool ok = true;
        for (size_t i = 5; i < spec.size(); i++)
            if (!isdigit(static_cast<unsigned char>(spec[i]))) { ok = false; break; }
        if (ok) {
            int n = std::stoi(spec.substr(5));
            if (n >= 0 && n <= 255) {
                char buf[24];
                snprintf(buf, sizeof(buf), "\033[48;5;%dm", n);
                return buf;
            }
        }
    }

    if (spec.size() == 7 && spec[0] == '#') {
        bool hex_ok = true;
        for (int i = 1; i <= 6; i++)
            if (!isxdigit(static_cast<unsigned char>(spec[i]))) { hex_ok = false; break; }
        if (hex_ok) {
            unsigned int r = 0, g = 0, b = 0;
            sscanf(spec.c_str() + 1, "%2x%2x%2x", &r, &g, &b);
            char buf[32];
            snprintf(buf, sizeof(buf), "\033[48;2;%u;%u;%um", r, g, b);
            return buf;
        }
    }

    return "";
}

/**
 * Parse a :color target name → enum.
 */
ColorTarget parse_color_target(const std::string& name) {
    if (name == "text"         || name == "fg")          return COLOR_TARGET_TEXT;
    if (name == "background"   || name == "bg")          return COLOR_TARGET_BACKGROUND;
    if (name == "statusbar"    || name == "status_bar"   ||
        name == "status")                                 return COLOR_TARGET_STATUS_BAR;
    if (name == "statustext"   || name == "status_text") return COLOR_TARGET_STATUS_TEXT;
    if (name == "comment")                               return COLOR_TARGET_COMMENT;
    if (name == "linenumber"   || name == "line_number"  ||
        name == "gutter")                                 return COLOR_TARGET_LINE_NUMBER;
    if (name == "currentline"  || name == "current_line" ||
        name == "curline")                                return COLOR_TARGET_CURRENT_LINE;
    return COLOR_TARGET_UNKNOWN;
}

// ─────────────────────────────────────────────────────────────────────────────
// write helpers
// ─────────────────────────────────────────────────────────────────────────────

static void write_str(const std::string& s) {
    if (!s.empty()) twrite(STDOUT_FILENO, s.c_str(), s.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Row drawing
// ─────────────────────────────────────────────────────────────────────────────

void Renderer::draw_rows(const EditorConfig& config, const Buffer& buffer) {
    std::string text_fg      = resolve_fg(config.text_color);
    std::string area_bg      = resolve_bg(config.background_color);
    std::string comment_fg   = resolve_fg(config.comment_color);
    std::string linenum_fg   = resolve_fg(config.line_number_color.empty()
                                          ? "cyan" : config.line_number_color);
    std::string curline_bg   = resolve_bg(config.current_line_color.empty()
                                          ? "color236" : config.current_line_color);

    for (int y = 0; y < config.screen_rows; y++) {
        int file_row = y + config.row_offset;
        bool is_current = config.highlight_current_line && (file_row == config.cursor_y);

        // Background for this row
        if (is_current) {
            write_str(curline_bg);
        } else {
            write_str(area_bg);
        }

        // ── Line-number gutter ───────────────────────────────────────────────
        if (config.show_line_numbers) {
            char gutter[16];
            if (file_row < buffer.get_line_count()) {
                write_str(linenum_fg);
                snprintf(gutter, sizeof(gutter), "%4d ", file_row + 1);
            } else {
                write_str(text_fg);
                snprintf(gutter, sizeof(gutter), "     ");
            }
            twrite(STDOUT_FILENO, gutter, strlen(gutter));
        }

        // ── Line content ─────────────────────────────────────────────────────
        if (file_row >= buffer.get_line_count()) {
            if (config.show_tilde) {
                write_str(text_fg);
                twrite(STDOUT_FILENO, "~", 1);
            }
        } else {
            std::string line     = buffer.get_line(file_row);
            int line_len         = static_cast<int>(line.size());
            int visible_start    = config.col_offset;
            int visible_len      = line_len - visible_start;
            if (visible_len < 0)                visible_len = 0;
            if (visible_len > config.screen_cols) visible_len = config.screen_cols;

            if (visible_len > 0) {
                bool is_comment = config.syntax_highlighting &&
                                  (line.find('#')  == 0 ||
                                   line.find("//") == 0);
                write_str(is_comment ? comment_fg : text_fg);
                twrite(STDOUT_FILENO,
                       line.c_str() + visible_start,
                       static_cast<size_t>(visible_len));
            }

            if (line.empty() && config.show_tilde) {
                write_str(text_fg);
                twrite(STDOUT_FILENO, "~", 1);
            }
        }

        // Reset + clear rest of line + newline
        twrite(STDOUT_FILENO, COLOR_RESET, COLOR_RESET_LEN);
        twrite(STDOUT_FILENO, CLEAR_LINE,  CLEAR_LINE_LEN);
        twrite(STDOUT_FILENO, "\r\n", 2);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Status bar
// ─────────────────────────────────────────────────────────────────────────────

void Renderer::draw_status_bar(const EditorConfig& config, const Buffer& buffer) {
    std::string bar_bg   = resolve_bg(config.status_bar_color);
    std::string bar_fg   = resolve_fg(config.status_text_color.empty()
                                       ? "black" : config.status_text_color);

    write_str(bar_bg);
    write_str(bar_fg);

    std::string mode_str = (config.mode == INSERT_MODE) ? "INSERT" : "COMMAND";
    std::string fname    = config.filename.empty() ? "[No Name]" : config.filename;
    std::string modmark  = config.modified ? "*" : "";

    // Expand format placeholders
    std::string fmt = config.status_format;
    auto replace_all = [&](const std::string& tok, const std::string& val) {
        size_t pos = 0;
        while ((pos = fmt.find(tok, pos)) != std::string::npos) {
            fmt.replace(pos, tok.size(), val);
            pos += val.size();
        }
    };
    replace_all("%f",        fname);
    replace_all("%modified", modmark);
    replace_all("%m",        mode_str);

    // Left side
    int len = static_cast<int>(fmt.size());
    if (len > config.screen_cols) len = config.screen_cols;
    twrite(STDOUT_FILENO, fmt.c_str(), static_cast<size_t>(len));

    // Right side: line/total (right-aligned)
    char rhs[32];
    int rlen = snprintf(rhs, sizeof(rhs), " %d/%d ",
                        config.cursor_y + 1, buffer.get_line_count());

    while (len < config.screen_cols) {
        if (config.screen_cols - len == rlen) {
            twrite(STDOUT_FILENO, rhs, static_cast<size_t>(rlen));
            len += rlen;
            break;
        }
        twrite(STDOUT_FILENO, " ", 1);
        len++;
    }

    twrite(STDOUT_FILENO, COLOR_RESET, COLOR_RESET_LEN);
    twrite(STDOUT_FILENO, "\r\n", 2);
}

// ─────────────────────────────────────────────────────────────────────────────
// Message bar
// ─────────────────────────────────────────────────────────────────────────────

void Renderer::draw_message_bar(const EditorConfig& config) {
    twrite(STDOUT_FILENO, CLEAR_LINE, CLEAR_LINE_LEN);

    int msglen = static_cast<int>(config.status_msg.size());
    if (msglen > config.screen_cols) msglen = config.screen_cols;

    if (msglen > 0 && (time(nullptr) - config.status_msg_time) < 5) {
        twrite(STDOUT_FILENO, config.status_msg.c_str(), static_cast<size_t>(msglen));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Full screen refresh  (only called when needs_redraw is true)
// ─────────────────────────────────────────────────────────────────────────────

void Renderer::refresh_screen(const EditorConfig& config, const Buffer& buffer) {
    EditorConfig mcfg = config;
    scroll(mcfg, buffer);

    terminal.hide_cursor();
    terminal.set_cursor_position(0, 0);

    // Fill background before drawing rows
    write_str(resolve_bg(config.background_color));

    draw_rows(mcfg, buffer);
    draw_status_bar(mcfg, buffer);
    draw_message_bar(mcfg);

    // Position cursor
    int gutter_offset = mcfg.show_line_numbers ? 5 : 0;
    int screen_x = (mcfg.cursor_x - mcfg.col_offset) + gutter_offset;
    int screen_y =  mcfg.cursor_y - mcfg.row_offset;
    terminal.set_cursor_position(screen_x, screen_y);

    terminal.show_cursor();

    // Propagate scroll offsets back to the global config
    editor_config.row_offset = mcfg.row_offset;
    editor_config.col_offset = mcfg.col_offset;
}

// ─────────────────────────────────────────────────────────────────────────────
// Scroll calculation
// ─────────────────────────────────────────────────────────────────────────────

void Renderer::scroll(EditorConfig& config, const Buffer& buffer) {
    int lc = buffer.get_line_count();
    if (config.cursor_y >= lc) config.cursor_y = lc - 1;
    if (config.cursor_y < 0)   config.cursor_y = 0;

    int ll = static_cast<int>(buffer.get_line(config.cursor_y).size());
    if (config.cursor_x > ll) config.cursor_x = ll;
    if (config.cursor_x < 0)  config.cursor_x = 0;

    if (config.cursor_y < config.row_offset)
        config.row_offset = config.cursor_y;
    if (config.cursor_y >= config.row_offset + config.screen_rows)
        config.row_offset = config.cursor_y - config.screen_rows + 1;

    if (config.cursor_x < config.col_offset)
        config.col_offset = config.cursor_x;
    if (config.cursor_x >= config.col_offset + config.screen_cols)
        config.col_offset = config.cursor_x - config.screen_cols + 1;
}