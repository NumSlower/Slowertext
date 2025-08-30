#!/bin/bash

# Configuration Testing Script for SlowerText Editor
# This script helps test and debug configuration loading

echo "SlowerText Configuration Test"
echo "============================="

# Check if config directory exists
CONFIG_DIR="$HOME/.config/slowertext"
CONFIG_FILE="$CONFIG_DIR/slowertextrc"

echo "1. Checking configuration directory..."
if [ ! -d "$CONFIG_DIR" ]; then
    echo "   Creating configuration directory: $CONFIG_DIR"
    mkdir -p "$CONFIG_DIR"
else
    echo "   Configuration directory exists: $CONFIG_DIR"
fi

echo ""
echo "2. Checking configuration file..."
if [ ! -f "$CONFIG_FILE" ]; then
    echo "   Configuration file not found. Creating sample config..."
    
    cat > "$CONFIG_FILE" << 'EOF'
# SlowerText Editor Test Configuration File
# =========================================

# Display Settings
show_line_numbers = true
show_tilde = true
highlight_current_line = false
syntax_highlighting = true

# Indentation and Formatting  
tab_width = 3
auto_indent = true

# Color Scheme
text_color = white
background_color = black
status_bar_color = cyan
comment_color = green

# Status Bar Configuration
status_format = %f%modified - %m

# Editor Behavior
confirm_quit = true
debug_mode = true

# Key Bindings
enter_insert = ctrl+i
enter_command = escape
save_file = ctrl+s
quit_editor = ctrl+q
force_quit = ctrl+f
cursor_up = arrow_up
cursor_down = arrow_down
cursor_left = arrow_left
cursor_right = arrow_right
EOF
    
    echo "   Created sample configuration: $CONFIG_FILE"
    echo "   You can edit this file to customize your settings."
else
    echo "   Configuration file exists: $CONFIG_FILE"
fi

echo ""
echo "3. Current configuration content:"
echo "   ------------------------------"
cat "$CONFIG_FILE" | grep -v '^#' | grep -v '^$' | head -10
echo "   ..."
echo ""

echo "4. Testing configuration paths..."
CONFIG_PATHS=(
    "$HOME/.config/slowertext/slowertextrc"
    "$HOME/.slowertextrc"
    "runtime/slowertextrc"
    "/etc/slowertext/slowertextrc"
)

for path in "${CONFIG_PATHS[@]}"; do
    if [ -f "$path" ]; then
        echo "   ✓ Found: $path"
    else
        echo "   ✗ Not found: $path"
    fi
done

echo ""
echo "5. Building and testing editor..."
if [ -f "Makefile" ]; then
    echo "   Building project..."
    make clean > /dev/null 2>&1
    make > /dev/null 2>&1
    
    if [ -f "bin/slowertext" ]; then
        echo "   ✓ Build successful"
        echo ""
        echo "6. Testing configuration loading..."
        echo "   Creating test file for editor..."
        
        # Create a test file
        cat > test_config.txt << 'EOF'
This is a test file for SlowerText editor.
You should see:
- Tab width set to 3 spaces (check by pressing Tab)
- Line numbers enabled (if configured)
- Debug messages (if debug_mode = true)

Try pressing:
- Tab (should insert 3 spaces)
- Ctrl+I (insert mode)
- Escape (command mode)
- Ctrl+S (save)
- Ctrl+Q (quit)

Configuration should load from:
~/.config/slowertext/slowertextrc
EOF
        
        echo "   Test file created: test_config.txt"
        echo ""
        echo "7. Instructions:"
        echo "   - Run './bin/slowertext test_config.txt' to test"
        echo "   - The editor should show debug messages if debug_mode=true"
        echo "   - Tab key should insert 3 spaces (as configured)"
        echo "   - Try editing $CONFIG_FILE and restart editor to see changes"
        echo ""
        echo "8. Quick config change test:"
        echo "   To test if config changes work without recompiling:"
        echo "   1. Edit $CONFIG_FILE"
        echo "   2. Change tab_width to a different value (e.g., 8)"
        echo "   3. Save the config file"
        echo "   4. Run the editor again - it should use the new tab width"
        echo ""
    else
        echo "   ✗ Build failed"
    fi
else
    echo "   ✗ Makefile not found"
fi

echo "Configuration test complete!"