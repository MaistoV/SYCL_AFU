#!/bin/bash

# Simple power measurement of Hitek xC220 cards using OPAE-SDK
output_file=power.csv
if [[ "$1" != "" ]]; then
    output_file=$1
fi

# Smaple and dump to tmp file
tmp_file=fpgainfo_power.tmp
fpgainfo power > $tmp_file

# Get timestamp
timestamp=$(date +"%s.%N")

# Grep from file
# Assuming only PCIe power supply, grep 12V and 3.3V power rails
volt_12V=$(grep "PCIe 12V Power Monitor 2 Voltage" $tmp_file | awk '{print $10}' | sed "s/,/./g")
amp_12V=$(grep "PCIe 12V Power Monitor 1 Current" $tmp_file  | awk '{print $10}' | sed "s/,/./g")
volt_3V3=$(grep "PCIe 3V3 Power Monitor 2 Voltage" $tmp_file | awk '{print $9}'  | sed "s/,/./g")
amp_3V2=$(grep "PCIe 3V3 Power Monitor 1 Current" $tmp_file  | awk '{print $9}'  | sed "s/,/./g")

# Append raw measures to file
# Create if does not exist
if [ ! -e $output_file ]; then
    echo "Time(s),12V Voltage(V), 12V Current(A), 3V3 Voltage(V), 3V3 Current(A)" > $output_file
fi
# Expecting header "12V Voltage, 12V Current, 3V3 Voltage, 3V3 Current"
echo "$timestamp,$volt_12V,$amp_12V,$volt_3V3,$amp_3V2" >> $output_file

# Remove tmp file
rm $tmp_file