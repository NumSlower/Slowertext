#include "../include/slowertext.h"
#include <cstring>
#include <ctime>

/**
 * Convert color name to ANSI escape code
 * @param color Color name string
 * @return ANSI color code string
 */
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

/**
 * Draw text rows on the screen
 * Handles line numbers, syntax highlighting, and current line highlighting
 * @param config Editor configuration
 * @param buffer Text buffer to display
 */
void Renderer::draw_rows(const EditorConfig& config, const Buffer& buffer) {
    std::string text_color = get_color_code(config.text_color);
    std::string bg_color = get_color_code("bg_" + config.background_color);
    std::string comment_color = get_color_code(config.comment_color);
    
    for (int y = 0; y < config.screen_rows; y++) {
        int file_row = y + config.row_offset;
        
        // Apply background color and highlight current line if enabled
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
        
        // Draw line content or tilde for empty lines
        if (file_row >= buffer.get_line_count()) {
            // Line is beyond buffer content
            if (config.show_tilde) {
                write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                write(STDOUT_FILENO, "~", 1);
            }
        } else {
            // Get line content and apply horizontal scrolling
            std::string line = buffer.get_line(file_row);
            int len = static_cast<int>(line.length()) - config.col_offset;
            if (len < 0) len = 0;
            if (len > config.screen_cols) len = config.screen_cols;
            
            if (len > 0) {
                // Apply basic syntax highlighting for comments
                if (config.syntax_highlighting && 
                    (line.find("#") == 0 || line.find("//") == 0)) {
                    write(STDOUT_FILENO, comment_color.c_str(), comment_color.length());
                } else {
                    write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                }
                
                // Write visible portion of line
                write(STDOUT_FILENO, line.c_str() + config.col_offset, len);
            }
            
            // Show tilde for empty lines if enabled
            if (line.empty() && config.show_tilde) {
                write(STDOUT_FILENO, text_color.c_str(), text_color.length());
                write(STDOUT_FILENO, "~", 1);
            }
        }

        // Reset colors and clear to end of line
        write(STDOUT_FILENO, COLOR_RESET, 4);
        write(STDOUT_FILENO, CLEAR_LINE, 3);
        write(STDOUT_FILENO, "\r\n", 2);
    }
}

/**
 * Draw status bar showing file info and editor mode
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void Renderer::draw_status_bar(const EditorConfig& config, const Buffer& buffer) {
    // Set status bar background color
    std::string status_color = get_color_code("bg_" + config.status_bar_color);
    write(STDOUT_FILENO, status_color.c_str(), status_color.length());
    
    char status[256];
    char rstatus[80];
    
    // Prepare status components
    std::string mode_str = (config.mode == INSERT_MODE) ? "INSERT" : "COMMAND";
    std::string filename = config.filename.empty() ? "[No Name]" : config.filename;
    std::string modified_indicator = config.modified ? "*" : "";
    
    // Process status format string with variable substitution
    std::string format = config.status_format;
    size_t pos = 0;
    
    // Replace %f with filename
    while ((pos = format.find("%f", pos)) != std::string::npos) {
        format.replace(pos, 2, filename);
        pos += filename.length();
    }
    
    // Replace %modified with modification indicator
    pos = 0;
    while ((pos = format.find("%modified", pos)) != std::string::npos) {
        format.replace(pos, 9, modified_indicator);
        pos += modified_indicator.length();
    }
    
    // Replace %m with mode
    pos = 0;
    while ((pos = format.find("%m", pos)) != std::string::npos) {
        format.replace(pos, 2, mode_str);
        pos += mode_str.length();
    }
    
    // Format left side of status bar
    int len = snprintf(status, sizeof(status), "%.240s", format.c_str());
    
    // Format right side with cursor position
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", 
                       config.cursor_y + 1, buffer.get_line_count());
    
    // Ensure status doesn't exceed screen width
    if (len > config.screen_cols) len = config.screen_cols;
    write(STDOUT_FILENO, status, len);
    
    // Fill middle with spaces and add right-aligned position info
    while (len < config.screen_cols) {
        if (config.screen_cols - len == rlen) {
            write(STDOUT_FILENO, rstatus, rlen);
            break;
        } else {
            write(STDOUT_FILENO, " ", 1);
            len++;
        }
    }
    
    // Reset colors and add newline
    write(STDOUT_FILENO, COLOR_RESET, 4);
    write(STDOUT_FILENO, "\r\n", 2);
}

/**
 * Draw message bar at bottom of screen
 * Shows status messages with timeout
 * @param config Editor configuration
 */
void Renderer::draw_message_bar(const EditorConfig& config) {
    write(STDOUT_FILENO, CLEAR_LINE, 3);
    
    int msglen = static_cast<int>(config.status_msg.length());
    if (msglen > config.screen_cols) msglen = config.screen_cols;
    
    // Show message only if it's recent (within 5 seconds)
    if (msglen && time(nullptr) - config.status_msg_time < 5) {
        write(STDOUT_FILENO, config.status_msg.c_str(), msglen);
    }
}

/**
 * Refresh entire screen
 * Orchestrates drawing of all screen elements
 * @param config Editor configuration
 * @param buffer Text buffer
 */
void Renderer::refresh_screen(const EditorConfig& config, const Buffer& buffer) {
    // Create mutable copy for scroll calculations
    EditorConfig mutable_config = config;
    scroll(mutable_config, buffer);
    
    // Hide cursor during refresh to prevent flicker
    terminal.hide_cursor();
    terminal.set_cursor_position(0, 0);
    
    // Set background color
    std::string bg_color = get_color_code("bg_" + config.background_color);
    write(STDOUT_FILENO, bg_color.c_str(), bg_color.length());
    
    // Draw all screen elements
    draw_rows(mutable_config, buffer);
    draw_status_bar(mutable_config, buffer);
    draw_message_bar(mutable_config);
    
    // Position cursor and show it
    int cursor_screen_x = (mutable_config.cursor_x - mutable_config.col_offset) + 
                         (config.show_line_numbers ? 5 : 0);
    int cursor_screen_y = (mutable_config.cursor_y - mutable_config.row_offset);
    
    terminal.set_cursor_position(cursor_screen_x, cursor_screen_y);
    terminal.show_cursor();
    
    // Update global config with scroll offsets
    editor_config.row_offset = mutable_config.row_offset;
    editor_config.col_offset = mutable_config.col_offset;
}

/**
 * Handle scrolling logic to keep cursor visible
 * @param config Editor configuration (modified with new scroll offsets)
 * @param buffer Text buffer
 */
void Renderer::scroll(EditorConfig& config, const Buffer& buffer) {
    // Ensure cursor stays within buffer bounds
    if (config.cursor_y >= buffer.get_line_count()) {
        config.cursor_y = buffer.get_line_count() - 1;
    }
    if (config.cursor_y < 0) {
        config.cursor_y = 0;
    }
    
    // Ensure cursor x position is valid for current line
    int line_length = static_cast<int>(buffer.get_line(config.cursor_y).length());
    if (config.cursor_x > line_length) {
        config.cursor_x = line_length;
    }
    if (config.cursor_x < 0) {
        config.cursor_x = 0;
    }

    // Adjust vertical scroll offset
    if (config.cursor_y < config.row_offset) {
        config.row_offset = config.cursor_y;
    }
    if (config.cursor_y >= config.row_offset + config.screen_rows) {
        config.row_offset = config.cursor_y - config.screen_rows + 1;
    }
    
    // Adjust horizontal scroll offset
    if (config.cursor_x < config.col_offset) {
        config.col_offset = config.cursor_x;
    }
    if (config.cursor_x >= config.col_offset + config.screen_cols) {
        config.col_offset = config.cursor_x - config.screen_cols + 1;
    }
}