#include "../include/slowertext.h"
#include <fstream>
#include <sys/stat.h>

bool FileManager::load_file(const std::string& filename, Buffer& buffer) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    buffer.clear();
    std::string line;
    bool first_line = true;

    while (std::getline(file, line)) {
        if (first_line) {
            buffer.set_line(0, line);
            first_line = false;
        } else {
            int line_count = buffer.get_line_count();
            buffer.set_line(line_count, line);
        }
    }

    file.close();
    buffer.set_modified(false);
    return true;
}

bool FileManager::save_file(const std::string& filename, const Buffer& buffer) {
    if (filename.empty()) {
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    const auto& lines = buffer.get_lines();
    for (size_t i = 0; i < lines.size(); ++i) {
        file << lines[i];
        if (i < lines.size() - 1) {
            file << '\n';
        }
    }

    file.close();
    return true;
}

bool FileManager::file_exists(const std::string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}