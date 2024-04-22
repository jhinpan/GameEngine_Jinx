#!/bin/bash

# Define possible HTML shell templates
declare -A shells
shells[1]="$(pwd)/retro_shell.html"
shells[2]="$(pwd)/minimal_shell.html"
shells[3]="$(pwd)/dark_shell.html"
shells[4]="$(pwd)/interactive_shell.html"

if [ -z "$1" ]; then
  echo "Select the HTML shell template for compilation:"
  echo "1: Retro Shell"
  echo "2: Minimal Shell"
  echo "3: Dark Shell"
  echo "4: Interactive Shell"

  read -p "Enter your choice (1-4): " choice
else
  choice=$1
fi

selected_shell=${shells[$choice]}

if [ -z "$selected_shell" ]; then
  echo "Invalid selection. Exiting."
  exit 1
fi

# Run the setup and build script with the chosen shell template
./setup_and_build.sh "$selected_shell"
