#!/bin/bash

# Define the directory to start the search from, default is the current directory
SEARCH_DIR="${1:-.}"

# Function to process each .framework found
process_framework() {
    local framework_path="$1"
    local framework_name="$(basename "$framework_path" .framework)"
    local library_path="$framework_path/Versions/A/$framework_name"

    echo "Fixing $framework_name"

    # Check if the library file exists
    if [[ -f "$library_path" ]]; then
        # Run codesign command on the library
        codesign --force --deep --sign - "$library_path"
        echo "$framework_name has been signed."
    else
        echo "Library not found for $framework_name"
    fi
}

# Export the function so it's available to find -exec
export -f process_framework

# Find all .framework directories and process them
find "$SEARCH_DIR" -name '*.framework' -type d -exec bash -c 'process_framework "$0"' {} \;

echo "All frameworks fixed."
