#!/bin/bash

# TLB Size Measurement Script
# Varies page count from 1 to 2048, doubling each time

# Number of trials - adjust based on your system
# More trials = more reliable but slower
# 10 million is a good starting point
TRIALS=1000000

# Compile the program if needed
if [ ! -f ./tlb ]; then
    echo "Compiling tlb.c..."
    gcc -O0 tlb.c -o tlb -lrt
    if [ $? -ne 0 ]; then
        echo "Compilation failed!"
        exit 1
    fi
fi

echo "TLB Size Measurement Results"
echo "============================"
echo "Trials per test: $TRIALS"
echo ""
echo "Pages | Time (ns)"
echo "------|----------"

# Test with powers of 2 from 1 to 2048
for pages in 1 2 4 8 16 32 64 128 256 512 1024 2048; do
    # Run the measurement
    output=$(./tlb $pages $TRIALS 2>/dev/null)
    
    # Extract the time value (third line)
    time_ns=$(echo "$output" | sed -n '3p' | awk '{print $1}')
    
    # Print formatted result
    printf "%5d | %8.2f\n" $pages $time_ns
done

echo ""
echo "Look for jumps in access time to identify:"
echo "- First TLB size (first jump)"
echo "- Second TLB size (second jump)"