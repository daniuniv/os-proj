#!/bin/bash

# File path and directory name
file="/home/tiege/Desktop/os/proj/test/intri//da.txt"
dirname="/home/tiege/Desktop/os/proj/test/intri/"

# Debugging message
echo "Script started for $file"

# Define thresholds
MIN_LINES=3
MAX_WORDS=1000
MAX_CHARACTERS=2000

# Function to count lines, words, and characters in a file
count_stats() {
    local file="$1"
    local lines=$(wc -l < "$file")
    local words=$(wc -w < "$file")
    local characters=$(wc -c < "$file")
    echo "$lines $words $characters"
}

# Function to move file to contaminated folder
move_to_contaminated() {
    local file="$1"
    local dirname="$2"
    mkdir -p "$dirname/contaminated"
    mv "$file" "$dirname/contaminated/"
}

# Main script execution
# Count lines, words, and characters
stats=$(count_stats "$file")
read -r lines words characters <<< "$stats"

# Debugging message
echo "File stats: $lines lines, $words words, $characters characters"

# Check conditions and move file if necessary
if (( lines < MIN_LINES )) && (( words > MAX_WORDS )) && (( characters > MAX_CHARACTERS )); then
    move_to_contaminated "$file" "$dirname"
    echo "File moved to contaminated directory"
fi
