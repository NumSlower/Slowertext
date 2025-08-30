# Makefile for slowertext

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -O2
DEBUG_FLAGS = -g -DDEBUG
INCLUDE_DIR = include
SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = $(BIN_DIR)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/slowertext

# Default target
all: $(TARGET)

# Create directories
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Link object files to create executable
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	$(CXX) $(OBJECTS) -o $@

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Debug build
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Clean build artifacts
clean:
	rm -rf $(BIN_DIR)

# Install target
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/slowertext
	chmod +x /usr/local/bin/slowertext
	@echo "Installing default config file..."
	@mkdir -p $(HOME)/.config/slowertext
	@if [ ! -f $(HOME)/.config/slowertext/slowertextrc ]; then \
		cp runtime/slowertextrc $(HOME)/.config/slowertext/slowertextrc; \
		chmod 644 $(HOME)/.config/slowertext/slowertextrc; \
		echo "Config file installed to $(HOME)/.config/slowertext/slowertextrc"; \
	else \
		echo "Config file already exists at $(HOME)/.config/slowertext/slowertextrc"; \
	fi

# Uninstall target
uninstall:
	rm -f /usr/local/bin/slowertext
	@echo "Note: Config file $(HOME)/.config/slowertext/slowertextrc was not removed"

# Run the program
run: $(TARGET)
	./$(TARGET)

# Run with a test file
test: $(TARGET)
	./$(TARGET) test.txt

# Check for memory leaks (requires valgrind)
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Static analysis (requires cppcheck)
check:
	cppcheck --enable=all --std=c++17 -I$(INCLUDE_DIR) $(SRC_DIR)

# Format code (requires clang-format)
format:
	clang-format -i $(SRC_DIR)/*.cpp $(INCLUDE_DIR)/*.h

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build the project (default)"
	@echo "  debug     - Build with debug flags"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install to /usr/local/bin and setup config"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  run       - Build and run the program"
	@echo "  test      - Build and run with test.txt"
	@echo "  memcheck  - Run with valgrind memory checker"
	@echo "  check     - Run static analysis with cppcheck"
	@echo "  format    - Format code with clang-format"
	@echo "  help      - Show this help message"

# Phony targets
.PHONY: all debug clean install uninstall run test memcheck check format help

# Dependencies
$(OBJ_DIR)/main.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/buffer.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/terminal.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/renderer.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/input.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/file.o: $(INCLUDE_DIR)/slowertext.h
$(OBJ_DIR)/config.o: $(INCLUDE_DIR)/slowertext.h