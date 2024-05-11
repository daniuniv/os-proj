#!/bin/bash

# File path and directory name
file="/home/tiege/Desktop/os/proj/test//da.txt"
dirname="/home/tiege/Desktop/os/proj/test/"

# Debugging message
echo "Script started for $file"

# Define thresholds
MIN_LINES=3
MAX_WORDS=1000
MAX_CHARACTERS=2000

# Function to count lines, words, characters, non-ASCII, and specific keywords in a file
count_stats() {
    if [[ $(tail -c 1 "$file") != $'\n' ]]; then
        echo >> "$file"
    fi
    local file="$1"
    local lines=$(wc -l < "$file")
    local words=$(wc -w < "$file")
    local characters=$(wc -c < "$file")
    local non_ascii=$(grep -P "[^\x00-\x7F]" "$file" | wc -l)
    local keywords=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
    local keyword_count=0
    for keyword in "${keywords[@]}"; do
        keyword_count=$((keyword_count + $(grep -oi "$keyword" "$file" | wc -l)))
    done
    echo "$lines $words $characters $non_ascii $keyword_count"
}
# Function to move file to contaminated folder
move_to_contaminated() {
    local file="$1"
    local dirname="$2"
    mkdir -p "$dirname/contaminated"
    mv "$file" "$dirname/contaminated/"
}

# Main script execution
# Count lines, words, characters, non-ASCII, and specific keywords
stats=$(count_stats "$file")
read -r lines words characters non_ascii keyword_count <<< "$stats"

# Debugging message
echo "File stats: $lines lines, $words words, $characters characters, $non_ascii non-ASCII characters, $keyword_count keywords found"

# Check conditions and move file if necessary
if (( lines < MIN_LINES )) && (( words > MAX_WORDS )) && (( characters > MAX_CHARACTERS )) || (( non_ascii > 0 )) || (( keyword_count > 0 )); then
    move_to_contaminated "$file" "$dirname"
    echo "File moved to contaminated directory"
fi
