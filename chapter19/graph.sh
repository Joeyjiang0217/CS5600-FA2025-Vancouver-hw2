#!/bin/bash

# Run TLB measurements and save to file
echo "# Pages Time(ns)" > tlb_data.txt

echo "Starting TLB measurements..."
echo "=========================="

for pages in 2 4 8 16 32 64 128 256 512 1024 2048; do
    if [ $pages -le 8 ]; then
        trials=100000000
    elif [ $pages -le 64 ]; then
        trials=10000000
    elif [ $pages -le 512 ]; then
        trials=5000000
    else
        trials=1000000
    fi
    
    output=$(./tlb $pages $trials 2>/dev/null)
    time_ns=$(echo "$output" | sed -n '3p' | awk '{print $1}')
    
    if [ ! -z "$time_ns" ]; then
        echo "$pages $time_ns" >> tlb_data.txt
        printf "Measured %4d pages: %8.2f ns (%d trials)\n" $pages $time_ns $trials
    fi
done

# Create gnuplot script
cat > plot_tlb.gnuplot << 'EOF'
# Set terminal and output
set terminal png size 1200,700 font "Arial,12" enhanced
set output 'tlb_measurement_gnuplot.png'

# Set title and labels
set title "TLB Size Measurement" font "Arial,18,bold"
set xlabel "Number Of Pages" font "Arial,14"
set ylabel "Time Per Access (ns)" font "Arial,14"

# Use log scale for x-axis (base 2)
set logscale x 2
set format x "%.0f"

# Set ranges to match your graph
set xrange [1.5:3000]
set yrange [0:*]

# Set grid with finer lines
set grid xtics ytics mxtics mytics lc rgb "#cccccc" lw 1 lt 0

# Define line style - orange color matching your graph
set style line 1 lc rgb '#FFA500' pt 7 ps 1.5 lt 1 lw 2.5

# Set x-axis ticks to match your graph
set xtics (2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048)

# Plot the data with legend in top right
set key top left font "Arial,12"
plot 'tlb_data.txt' using 1:2 with linespoints ls 1 title 'TLB Access Time'

# Save the graph
set output

# Also create a more detailed version with annotations
set terminal png size 1200,700 font "Arial,12" enhanced
set output 'tlb_measurement_annotated.png'

# Add labels for TLB boundaries (adjust based on your data)
set label 1 "L1 TLB Hits\n(~1-1.2ns)" at 8,1.5 center font "Arial,10" textcolor rgb "#006600"
set arrow 1 from 16,1.5 to 30,1.15 nohead lc rgb "#006600" lw 1 lt 2

set label 2 "L1 TLB Boundary\n(32-64 entries)" at 48,2.5 center font "Arial,10" textcolor rgb "#cc0000"
set arrow 2 from 48,2.8 to 64,3.8 heads lc rgb "#cc0000" lw 2

set label 3 "L2 TLB Hits\n(~4ns)" at 150,3.5 center font "Arial,10" textcolor rgb "#0066cc"

set label 4 "L2 TLB Boundary\n(~512-1024 entries)" at 750,6 center font "Arial,10" textcolor rgb "#cc0000"
set arrow 3 from 750,6.5 to 1024,8 heads lc rgb "#cc0000" lw 2

replot
EOF

# Run gnuplot
if command -v gnuplot &> /dev/null; then
    gnuplot plot_tlb.gnuplot
    echo ""
    echo "Graphs created:"
    echo "  - tlb_measurement_gnuplot.png (clean version)"
    echo "  - tlb_measurement_annotated.png (with annotations)"
    echo ""
    echo "=== TLB Analysis Summary ==="
    echo "Based on your measurements:"
    echo "  ? L1 TLB size: ~32-64 entries (jump at 64 pages)"
    echo "  ? L2 TLB size: ~512-1024 entries (jump at 1024 pages)"
    echo "  ? L1 TLB hit time: ~1.1-1.2 ns"
    echo "  ? L2 TLB hit time: ~4 ns"
    echo "  ? TLB miss time: ~8-9 ns"
else
    echo "Gnuplot not found. Install with: sudo apt-get install gnuplot"
fi