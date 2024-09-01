#!/bin/bash

# Check if the correct number of arguments was provided
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <number_of_lines>"
    exit 1
fi

n=$1
#output_file=$2
dictionary="wordlist.10000"  # Path to the dictionary file

# Check if the dictionary file exists
if [ ! -f "$dictionary" ]; then
    echo "Dictionary file does not exist at $dictionary"
    exit 1
fi

# Generate the file with n random lines from the dictionary
# echo "Generated file '$output_file' with $n lines of text."
shuf -n "$n" "$dictionary" 


