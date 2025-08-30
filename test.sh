#!/bin/bash

# Debug test script for SlowerText
echo "SlowerText Debug Test Script"
echo "============================="

# Check if the runtime directory exists
if [ -d "runtime" ]; then
    echo "✓ runtime directory exists"
else
    echo "✗ runtime directory missing - creating it"
    mkdir -p runtime
fi

# Check if slowertextrc exists in runtime
if [ -f "runtime/slowertextrc" ]; then
    echo "✓ runtime/slowertextrc exists"
    echo "Contents:"
    head -10 runtime/slowertextrc
else
    echo "✗ runtime/slowertextrc missing - this might be the problem!"
    echo "Creating runtime/slowertextrc with proper configuration..."
    cat > runtime/slowertextrc << 'EOF'
# SlowerText Configuration File
show_line_numbers = true
tab_width = 4
auto_indent = true
show_whitespace = false
status_format = %f%modified - %m
text_color = white
background_color = black
status_bar_color = cyan
comment_color = green
show_tilde = true
highlight_current_line = true
confirm_quit = true
syntax_highlighting = true
debug_mode = true
default_mode = insert
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
    echo "✓ Created runtime/slowertextrc"
fi

echo ""
echo "Key things to check:"
echo "1. Make sure you have the proper slowertextrc file (not C++ code)"
echo "2. The tab_width should be set to 4 in the config"
echo "3. Try enabling debug_mode = true to see config loading"
echo ""
echo "To test tab functionality:"
echo "1. Compile with: make clean && make"
echo "2. Run with: ./bin/slowertext test.txt"
echo "3. Press Ctrl+I to enter insert mode"
echo "4. Press Tab - you should see 4 spaces inserted"
echo "5. The status bar should show [Tab:4]"

# Check if binary exists
if [ -f "bin/slowertext" ]; then
    echo ""
    echo "✓ Binary exists at bin/slowertext"
    echo "You can now test the editor!"
else
    echo ""
    echo "✗ Binary not found. Run 'make' to compile."
fi