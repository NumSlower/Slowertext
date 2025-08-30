#include "../include/slowertext.h"

/**
 * Buffer constructor
 * Initializes an empty buffer with one empty line
 */
Buffer::Buffer() : modified(false) {
    lines.push_back("");
}

/**
 * Insert a character at the specified position
 * @param x Column position (0-based)
 * @param y Row position (0-based) 
 * @param c Character to insert
 */
void Buffer::insert_char(int x, int y, char c) {
    // Validate row bounds
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    // Clamp column position to valid range
    if (x < 0 || x > static_cast<int>(lines[y].length())) {
        x = lines[y].length();
    }
    
    // Insert character and mark as modified
    lines[y].insert(x, 1, c);
    modified = true;
}

/**
 * Delete character at the specified position
 * @param x Column position (0-based)
 * @param y Row position (0-based)
 */
void Buffer::delete_char(int x, int y) {
    // Validate row bounds
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    // Validate column bounds
    if (x < 0 || x >= static_cast<int>(lines[y].length())) {
        return;
    }
    
    // Delete character and mark as modified
    lines[y].erase(x, 1);
    modified = true;
}

/**
 * Insert a new line at the specified position
 * Splits the current line at the cursor position
 * @param x Column position where to split the line
 * @param y Row position
 */
void Buffer::insert_newline(int x, int y) {
    // Validate row bounds
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    // Clamp column position to valid range
    if (x < 0 || x > static_cast<int>(lines[y].length())) {
        x = lines[y].length();
    }
    
    // Split the line at the cursor position
    std::string new_line = lines[y].substr(x);  // Text after cursor
    lines[y] = lines[y].substr(0, x);           // Text before cursor
    
    // Insert the new line after current line
    lines.insert(lines.begin() + y + 1, new_line);
    modified = true;
}

/**
 * Delete an entire line
 * @param y Row position to delete
 */
void Buffer::delete_line(int y) {
    // Validate row bounds
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    // If more than one line, delete the specified line
    if (lines.size() > 1) {
        lines.erase(lines.begin() + y);
        modified = true;
    } else {
        // If only one line, clear it instead of deleting
        lines[0] = "";
        modified = true;
    }
}

/**
 * Get the content of a specific line
 * @param y Row position
 * @return Line content as string, empty string if invalid row
 */
std::string Buffer::get_line(int y) const {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return "";
    }
    return lines[y];
}

/**
 * Get the total number of lines in the buffer
 * @return Number of lines
 */
int Buffer::get_line_count() const {
    return static_cast<int>(lines.size());
}

/**
 * Set the content of a specific line
 * Expands buffer with empty lines if necessary
 * @param y Row position
 * @param line New line content
 */
void Buffer::set_line(int y, const std::string& line) {
    // Invalid row position
    if (y < 0) {
        return;
    }
    
    // Expand buffer with empty lines if necessary
    while (y >= static_cast<int>(lines.size())) {
        lines.push_back("");
    }
    
    // Set line content and mark as modified
    lines[y] = line;
    modified = true;
}

/**
 * Check if the buffer has been modified since last save
 * @return True if modified, false otherwise
 */
bool Buffer::is_modified() const {
    return modified;
}

/**
 * Set the modification flag
 * @param mod New modification state
 */
void Buffer::set_modified(bool mod) {
    modified = mod;
}

/**
 * Clear all buffer content and reset to single empty line
 */
void Buffer::clear() {
    lines.clear();
    lines.push_back("");
    modified = false;
}

/**
 * Get const reference to all lines for read-only iteration
 * @return Const reference to lines vector
 */
const std::vector<std::string>& Buffer::get_lines() const {
    return lines;
}