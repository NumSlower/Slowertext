#include "../include/slowertext.h"

Buffer::Buffer() : modified(false) {
    lines.push_back("");
}

void Buffer::insert_char(int x, int y, char c) {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    if (x < 0 || x > static_cast<int>(lines[y].length())) {
        x = lines[y].length();
    }
    
    lines[y].insert(x, 1, c);
    modified = true;
}

void Buffer::delete_char(int x, int y) {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    if (x < 0 || x >= static_cast<int>(lines[y].length())) {
        return;
    }
    
    lines[y].erase(x, 1);
    modified = true;
}

void Buffer::insert_newline(int x, int y) {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    if (x < 0 || x > static_cast<int>(lines[y].length())) {
        x = lines[y].length();
    }
    
    std::string new_line = lines[y].substr(x);
    lines[y] = lines[y].substr(0, x);
    lines.insert(lines.begin() + y + 1, new_line);
    modified = true;
}

void Buffer::delete_line(int y) {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return;
    }
    
    if (lines.size() > 1) {
        lines.erase(lines.begin() + y);
        modified = true;
    } else {
        lines[0] = "";
        modified = true;
    }
}

std::string Buffer::get_line(int y) const {
    if (y < 0 || y >= static_cast<int>(lines.size())) {
        return "";
    }
    return lines[y];
}

int Buffer::get_line_count() const {
    return static_cast<int>(lines.size());
}

void Buffer::set_line(int y, const std::string& line) {
    if (y < 0) {
        return;
    }
    
    while (y >= static_cast<int>(lines.size())) {
        lines.push_back("");
    }
    
    lines[y] = line;
    modified = true;
}

bool Buffer::is_modified() const {
    return modified;
}

void Buffer::set_modified(bool mod) {
    modified = mod;
}

void Buffer::clear() {
    lines.clear();
    lines.push_back("");
    modified = false;
}

const std::vector<std::string>& Buffer::get_lines() const {
    return lines;
}