# slowertext

A lightweight, Vim-like text editor written in C++ with minimal dependencies.

## Features

- **Two modes**: Insert mode (default) and Command mode
- **Vim-like navigation**: Arrow keys, cursor movement
- **File operations**: Open, save, save as, quit
- **Visual indicators**: 
  - `~` symbols on empty lines
  - `*` prefix on modified files
  - Mode indicator in status bar
- **No terminal flickering**: Optimized rendering
- **Error handling**: Robust file I/O and input handling
- **Clean exit**: Terminal state restored on exit

## Installation

### Prerequisites

- C++17 compatible compiler (g++, clang++)
- Make
- Unix-like system (Linux, macOS, WSL)

### Build

```bash
git clone https://github.com/NumSlower/Slowertext.git
cd slowertext
make
```

### Install System-wide

```bash
sudo make install
```

### Uninstall

```bash
sudo make uninstall
```

## Usage

### Starting the Editor

```bash
# Start with empty buffer
./bin/slowertext

# Open existing file or create new one
./bin/slowertext filename.txt
```

### Modes

#### Insert Mode (Default)
- **Enter**: `Ctrl + I` (if in Command mode)
- Type normally to insert text
- **Arrow keys**: Navigate cursor
- **Enter**: Insert new line
- **Backspace**: Delete character before cursor
- **Delete**: Delete character at cursor
- **ESC**: Switch to Command mode

#### Command Mode
- **Enter**: `ESC` (from Insert mode)
- **Arrow keys**: Navigate cursor (no text insertion)
- **Shift + ;** (colon): Enter command prompt
- **ESC**: Cancel command input

### Commands

All commands are entered after pressing `:` in Command mode:

- `q` - Quit (warns if unsaved changes)
- `q!` - Force quit (ignore unsaved changes)
- `s` or `w` - Save file
- `wq` or `sq` - Save and quit
- `saves <filename>` - Save as new filename

### Global Shortcuts

- **Ctrl + S**: Save file (works in any mode)

### Visual Indicators

- `~` - Empty lines beyond end of file
- `*filename.txt` - File has unsaved changes
- Status bar shows current mode and file info

## Project Structure

```
slowertext/
├── bin/                 # Compiled binaries and object files
├── include/
│   └── slowertext.h    # Header file with class declarations
├── runtime/
│   └── slowertextrc    # Configuration file
├── src/
│   ├── main.cpp        # Entry point
│   ├── buffer.cpp      # Text buffer management
│   ├── terminal.cpp    # Terminal control and raw mode
│   ├── renderer.cpp    # Screen rendering and display
│   ├── input.cpp       # Input handling and editor logic
│   └── file.cpp        # File operations and management
├── Makefile            # Build configuration
└── README.md           # This file
```

## Development

### Debug Build

```bash
make debug
```

### Memory Check

```bash
make memcheck  # requires valgrind
```

### Static Analysis

```bash
make check     # requires cppcheck
```

### Code Formatting

```bash
make format    # requires clang-format
```

### Clean Build

```bash
make clean
```

## Technical Details

### Terminal Handling

- Uses raw mode for immediate key response
- Escape sequences for cursor control and screen clearing
- No external terminal libraries (ncurses, etc.)
- Handles window resizing gracefully

### Rendering

- Double-buffered rendering prevents flicker
- Efficient screen updates using ANSI escape codes
- Scrolling support for large files
- Status bar with file and mode information

### Input Processing

- Non-blocking input with timeout
- Escape sequence parsing for special keys
- Separate handling for Insert and Command modes
- Robust error handling for terminal I/O

### File Operations

- Stream-based file I/O
- Automatic backup of original file permissions
- Error handling for file access issues
- Support for creating new files

## Keyboard Reference

### Insert Mode
| Key | Action |
|-----|--------|
| `Ctrl+I` | Switch to Insert mode |
| `ESC` | Switch to Command mode |
| `Ctrl+S` | Save file |
| `↑↓←→` | Move cursor |
| `Enter` | New line |
| `Backspace` | Delete previous character |
| `Delete` | Delete current character |

### Command Mode
| Key | Action |
|-----|--------|
| `Ctrl+I` | Switch to Insert mode |
| `:` | Enter command prompt |
| `ESC` | Cancel command |
| `↑↓←→` | Move cursor |

### Commands
| Command | Action |
|---------|--------|
| `:q` | Quit |
| `:q!` | Force quit |
| `:s` or `:w` | Save |
| `:wq` or `:sq` | Save and quit |
| `:saves <file>` | Save as |

## License

This project is open source. Feel free to modify and distribute.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Known Issues

- Large files (>100MB) may have performance impact
- Unicode support is basic
- No syntax highlighting
- No undo/redo functionality

## Future Enhancements

- Syntax highlighting
- Undo/redo system
- Search and replace
- Multiple buffers/tabs
- Configuration file support
- Plugin system# Slowertext
# Slowertext
