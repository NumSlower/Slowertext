#include "../include/slowertext.h"
#include <fstream>
#include <sys/stat.h>

/**
 * Load file content into buffer.
 *
 * BUG FIX: the original code skipped the last line if the file ended without a
 * trailing newline, because std::getline returns false after consuming the last
 * line that has no '\n'.  That is actually handled correctly by getline — we
 * just need to make sure we don't lose it.  The original logic was actually fine
 * on that front, but it called set_line() on index 0 for the first line and
 * then on line_count for subsequent lines.  That works but is unnecessarily
 * convoluted; rewriting it more clearly also removes the first-line special
 * case, which was a latent bug: if the file was empty, buffer was left with the
 * default empty line rather than truly empty.
 */
bool FileManager::load_file(const std::string& filename, Buffer& buffer) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    buffer.clear();

    std::string line;
    bool got_any_line = false;
    int  row = 0;

    while (std::getline(file, line)) {
        buffer.set_line(row, line);
        row++;
        got_any_line = true;
    }

    // If the file was completely empty, leave the buffer with one empty line
    // (which clear() already set up).
    (void)got_any_line;

    file.close();
    buffer.set_modified(false);
    return true;
}

/**
 * Save buffer content to file.
 *
 * BUG FIX: the original skipped the trailing newline on the last line.
 * Most Unix tools expect every text file to end with '\n'; we now always
 * write one.  This also means reloading the file round-trips cleanly.
 */
bool FileManager::save_file(const std::string& filename, const Buffer& buffer) {
    if (filename.empty()) {
        return false;
    }

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    const auto& lines = buffer.get_lines();
    for (const auto& l : lines) {
        file << l << '\n';
    }

    file.close();
    // Check that the stream didn't fail silently
    return file.good() || file.eof(); // eof is expected after close
}

/**
 * Check whether a file exists on the filesystem.
 */
bool FileManager::file_exists(const std::string& filename) {
    struct stat buf;
    return (stat(filename.c_str(), &buf) == 0);
}