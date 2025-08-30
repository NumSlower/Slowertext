#include "../include/slowertext.h"
#include <cstring>
#include <ctime>

std::string get_color_code(const std::string& color) {
    if (color == "black") return COLOR_BLACK;
    if (color == "red") return COLOR_RED;
    if (color == "green") return COLOR_GREEN;
    if (color == "yellow") return COLOR_YELLOW;
    if (color == "blue") return COLOR_BLUE;
    if (color == "magenta") return COLOR_MAGENTA;
    if (color == "cyan") return COLOR_CYAN;
    if (color == "white") return COLOR_WHITE;
    if (color == "bg_black") return BG_BLACK;
    if (color == "bg_red") return BG_RED;
    if (color == "bg_green") return BG_GREEN;
    if (color == "bg_yellow") return BG_YELLOW;
    if (color == "bg_blue") return BG_BLUE;
    if (color == "bg_magenta") return BG_MAGENTA;
    if (color == "bg_cyan") return BG_CYAN;
    if (color == "bg_white") return BG_WHITE;
    return "";
}

void Renderer::draw_rows(const EditorConfig& config, const Buffer& buffer) {
    std::string text_color = get_color_code(config.text_color);
    std::string bg_color = get_color_code("bg_" + config.background_color);
    std::string comment_color = get_color_code(config.comment_color);
    
    for (int y = 0; y < config.screen_rows; y++) {
        int file_row = y + config.row_offset;
        
        // Apply background color and highlight current line
        if (config.highlight_current_line && file_row == config.cursor_y) {
            write(STDOUT_FILENO, "\x1b[7m", 4); // Invert colors for current line
        } else {
            write(STDOUT_FILENO, bg_color.c_str(), bg_color.length());
        }
        
        // Draw line numbers if enabled
        if (config.show_line_numbers) {
            char line_num[16];
            if (file_row < buffer.get_line_count()) {
                snprintf(line_num, sizeof(line_num), "%4d ", file_row + 1);
            } else {
                snprintf(line_num, sizeof(line_num), "     ");
            }
            write(STDOUT_FILENO, line_num, strlen(line_num));
        }
        
        if (file_row >= buffer.get_line_count()) {
            if (config.show_tilde) {
                write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                write(STDOUT_FILENO, "~", 1);
            }
        } else {
            std::string line = buffer.get_line(file_row);
            int len = line.length() - config.col_offset;
            if (len < 0) len = 0;
            if (len > config.screen_cols) len = config.screen_cols;
            
            if (len > 0) {
                // Apply syntax highlighting for comments
                if (config.syntax_highlighting && (line.find("#") == 0 || line.find("//") == 0)) {
                    write(STDOUT_FILENO, comment_color.c_str(), comment_color.length());
                } else {
                    write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                }
                write(STDOUT_FILENO, line.c_str() + config.col_offset, len);
            }
            
            if (line.empty() && config.show_tilde) {
                write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                write(STDOUT_FILENO, "~", 1);
            }
        }

        write(STDOUT_FILENO, COLOR_RESET, 4);
        write(STDOUT_FILENO, CLEAR_LINE, 3);
        write(STDOUT_FILENO, "\r\n", 2);
    }
}

void Renderer::draw_status_bar(const EditorConfig& config, const Buffer& buffer) {
    std::string status_color = get_color_code("bg_" + config.status_bar_color);
    write(STDOUT_FILENO, status_color.c_str(), status_color.length());
    
    char status[256];
    char rstatus[80];
    
    std::string mode_str = (config.mode == INSERT_MODE) ? "INSERT" : "COMMAND";
    std::string filename = config.filename.empty() ? "[No Name]" : config.filename;
    std::string modified_indicator = config.modified ? "*" : "";
    
    std::string format = config.status_format;
    size_t pos = 0;
    while ((pos = format.find("%f", pos)) != std::string::npos) {
        format.replace(pos, 2, filename);
        pos += filename.length();
    }
    pos = 0;
    while ((pos = format.find("%modified", pos)) != std::string::npos) {
        format.replace(pos, 9, modified_indicator);
        pos += modified_indicator.length();
    }
    pos = 0;
    while ((pos = format.find("%m", pos)) != std::string::npos) {
        format.replace(pos, 2, mode_str);
        pos += mode_str.length();
    }
    
    int len = snprintf(status, sizeof(status), "%.240s", format.c_str());
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", config.cursor_y + 1, buffer.get_line_count());
    
    if (len > config.screen_cols) len = config.screen_cols;
    write(STDOUT_FILENO, status, len);
    
    while (len < config.screen_cols) {
        if (config.screen_cols - len == rlen) {
            write(STDOUT_FILENO, rstatus, rlen);
            break;
        } else {
            write(STDOUT_FILENO, " ", 1);
            len++;
        }
    }
    
    write(STDOUT_FILENO, COLOR_RESET, 4);
    write(STDOUT_FILENO, "\r\n", 2);
}

void Renderer::draw_message_bar(const EditorConfig& config) {
    write(STDOUT_FILENO, CLEAR_LINE, 3);
    int msglen = config.status_msg.length();
    if (msglen > config.screen_cols) msglen = config.screen_cols;
    if (msglen && time(nullptr) - config.status_msg_time < 5) {
        write(STDOUT_FILENO, config.status_msg.c_str(), msglen);
    }
}

void Renderer::refresh_screen(const EditorConfig& config, const Buffer& buffer) {
    EditorConfig mutable_config = config;
    scroll(mutable_config, buffer);
    
    terminal.hide_cursor();
    terminal.set_cursor_position(0, 0);
    
    std::string bg_color = get_color_code("bg_" + config.background_color);
    write(STDOUT_FILENO, bg_color.c_str(), bg_color.length());
    
    draw_rows(mutable_config, buffer);
    draw_status_bar(mutable_config, buffer);
    draw_message_bar(mutable_config);
    
    terminal.set_cursor_position(
        (mutable_config.cursor_x - mutable_config.col_offset) + (config.show_line_numbers ? 5 : 0),
        (mutable_config.cursor_y - mutable_config.row_offset)
    );
    terminal.show_cursor();
    
    editor_config.row_offset = mutable_config.row_offset;
    editor_config.col_offset = mutable_config.col_offset;
}

void Renderer::scroll(EditorConfig& config, const Buffer& buffer) {
    // Ensure cursor stays within buffer bounds
    if (config.cursor_y >= buffer.get_line_count()) {
        config.cursor_y = buffer.get_line_count() - 1;
    }
    if (config.cursor_y < 0) {
        config.cursor_y = 0;
    }
    int line_length = buffer.get_line(config.cursor_y).length();
    if (config.cursor_x > line_length) {
        config.cursor_x = line_length;
    }
    if (config.cursor_x < 0) {
        config.cursor_x = 0;
    }

    // Adjust scroll offsets
    if (config.cursor_y < config.row_offset) {
        config.row_offset = config.cursor_y;
    }
    if (config.cursor_y >= config.row_offset + config.screen_rows) {
        config.row_offset = config.cursor_y - config.screen_rows + 1;
    }
    if (config.cursor_x < config.col_offset) {
        config.col_offset = config.cursor_x;
    }
    if (config.cursor_x >= config.col_offset + config.screen_cols) {
        config.col_offset = config.cursor_x - config.screen_cols + 1;
    }
}