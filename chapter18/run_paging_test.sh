#!/usr/bin/env bash
set -u  # stop only if an undefined variable is used (not on errors)

echo "Starting paging-linear-translate tests..."
echo "========================================="
start_time=$(date)
echo "Started at: $start_time"
echo ""

commands=(
  "python3 ./paging-linear-translate.py -s 3 -a 1024m -p 512m -P 32m -c -v"
  "python3 ./paging-linear-translate.py -s 3 -a 100m  -p 512m -P 32m -c -v"
  "python3 ./paging-linear-translate.py -s 3 -a 15k   -p 512m -P 5k  -c -v"
  "python3 ./paging-linear-translate.py -s 3 -a 15k   -p 30k  -P 5k  -c -v"
  "python3 ./paging-linear-translate.py -s 3 -a 0     -p 30k  -P 5k  -c -v"
  "python3 ./paging-linear-translate.py -s 3 -a 2g    -p 4g   -P 1k  -c -v"
)

i=1
for cmd in "${commands[@]}"; do
  echo "[$i/${#commands[@]}] Running: $cmd"
  echo "----------------------------------------"

  # Run command, suppress all stderr (error output)
  eval "$cmd" 2>/dev/null
  
  echo "----------------------------------------"
  echo "Finished command #$i"
  echo ""
  ((i++))
done

echo "========================================="
echo "All tests completed!"
echo "Finished at: $(date)"
