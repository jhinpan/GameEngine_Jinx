#!/bin/bash

# Navigate into the emsdk directory
cd emsdk

# Install the latest version of Emscripten
emsdk install latest

# Activate the latest version of Emscripten
emsdk activate latest

# Source the Emscripten environment script
source ./emsdk_env.sh

# Return to the previous directory
cd ..

# Clean any previous build artifacts
make clean

# Build the project using the selected HTML shell file
make SHELL_FILE=$1

echo "Setup and build complete."
