# Makefile for SlowerText Editor
# A simple text editor with configurable settings

# Compiler and compilation flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2
DEBUG_FLAGS = -g -DDEBUG

# Directory structure
INCLUDE_DIR = include
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)

# Source files and object files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/slowertext

# Default target - build the editor
all: $(TARGET)

# Create binary directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link object files to create the final executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Debug build with debugging symbols and flags
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)
	@echo "Debug build complete"

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)
	@echo "Clean complete"

# Install the editor system-wide
install: $(TARGET)
	@echo "Installing SlowerText..."
	cp $(TARGET) /usr/local/bin/slowertext
	chmod +x /usr/local/bin/slowertext
	@echo "Installing default configuration..."
	@mkdir -p $(HOME)/.config/slowertext
	@if [ ! -f $(HOME)/.config/slowertext/slowertextrc ]; then \
		cp runtime/slowertextrc $(HOME)/.config/slowertext/slowertextrc; \
		chmod 644 $(HOME)/.config/slowertext/slowertextrc; \
		echo "Config file installed to $(HOME)/.config/slowertext/slowertextrc"; \
	else \
		echo "Config file already exists at $(HOME)/.config/slowertext/slowertextrc"; \
	fi
	@echo "Installation complete"

# Uninstall the editor
uninstall:
	rm -f /usr/local/bin/slowertext
	@echo "SlowerText uninstalled"
	@echo "Note: Config file $(HOME)/.config/slowertext/slowertextrc was not removed"

# Build and run the editor
run: $(TARGET)
	./$(TARGET)

# Build and run with a test file
test: $(TARGET)
	./$(TARGET) test.txt

# Memory leak check using valgrind (requires valgrind to be installed)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Static code analysis using cppcheck (requires cppcheck to be installed)
check:
	cppcheck --enable=all --std=c++17 -I$(INCLUDE_DIR) $(SRC_DIR)

# Format code using clang-format (requires clang-format to be installed)
format:
	clang-format -i $(SRC_DIR)/*.cpp $(INCLUDE_DIR)/*.h

# Display help information
help:
	@echo "SlowerText Editor Build System"
	@echo "=============================="
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug flags enabled"
	@echo "  clean     - Remove all build artifacts"
	@echo "  install   - Install to /usr/local/bin and setup config"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  run       - Build and run the editor"
	@echo "  test      - Build and run with test.txt"
	@echo "  memcheck  - Run with valgrind memory checker"
	@echo "  check     - Run static analysis with cppcheck"
	@echo "  format    - Format code with clang-format"
	@echo "  help      - Show this help message"

# Declare phony targets (targets that don't create files)
.PHONY: all debug clean install uninstall run test memcheck check format help

# Dependency declarations for header file changes
$(OBJ_DIR)/main.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/buffer.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/terminal.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/renderer.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/input.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/file.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/config.o: $(INCLUDE_DIR)/slowertext.h