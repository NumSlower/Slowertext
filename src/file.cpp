#include "../include/slowertext.h"
#include <fstream>
#include <sys/stat.h>

/**
 * Load file content into buffer
 * @param filename Path to file to load
 * @param buffer Buffer to populate with file content
 * @return True if file loaded successfully, false otherwise
 */
bool FileManager::load_file(const std::string& filename, Buffer& buffer) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Clear existing buffer content
    buffer.clear();
    
    std::string line;
    bool first_line = true;

    // Read file line by line
    while (std::getline(file, line)) {
        if (first_line) {
            // Replace the default empty line with first file line
            buffer.set_line(0, line);
            first_line = false;
        } else {
            // Append subsequent lines
            int line_count = buffer.get_line_count();
            buffer.set_line(line_count, line);
        }
    }

    file.close();
    
    // Mark buffer as unmodified since we just loaded from file
    buffer.set_modified(false);
    return true;
}

/**
 * Save buffer content to file
 * @param filename Path to file to save to
 * @param buffer Buffer containing content to save
 * @return True if file saved successfully, false otherwise
 */
bool FileManager::save_file(const std::string& filename, const Buffer& buffer) {
    if (filename.empty()) {
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    // Write all lines to file
    const auto& lines = buffer.get_lines();
    for (size_t i = 0; i < lines.size(); ++i) {
        file << lines[i];
        // Add newline after each line except the last one
        if (i < lines.size() - 1) {
            file << '\n';
        }
    }

    file.close();
    return true;
}

/**
 * Check if a file exists on the filesystem
 * @param filename Path to file to check
 * @return True if file exists, false otherwise
 */
bool FileManager::file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}