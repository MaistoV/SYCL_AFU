#!/bin/bash

output_file=$1/power.csv

# Create if does not exist
if [ ! -e $output_file ]; then
    echo "Time(s),12V Voltage(V), 12V Current(A), 3V3 Voltage(V), 3V3 Current(A)" > $output_file
fi

# Launch a busy loop, polling with fpgainfo
# NOTE: this has a round-trip-time of aboud 0.5 seconds
cnt=0
while (true) do
    cnt=$((cnt+1))
    echo "Collecting sample #$cnt, kill to stop"
    # Launch measure
    ${ROOT_DIR}/measures/power/measure_power.sh $output_file
done