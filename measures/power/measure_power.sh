#!/bin/bash

# Sysfs path of target hwmon
SYSFS_PATH=$1

# Simple power measurement of Hitek xC220 cards using OPAE-SDK
OUTPUT_FILE=power.csv
if [[ "$2" != "" ]]; then
    OUTPUT_FILE=$2
fi

# Get timestamp
timestamp=$(date +"%s.%N")

# Grep from file
# Assuming only PCIe power supply; cat data for 12V and 3.3V power rails
volt_12V=$(cat $SYSFS_PATH/in4_input)       # in4_label = PCIe 12V Power Monitor 2 Voltage
amp_12V=$(cat $SYSFS_PATH/curr3_input)      # curr3_label = PCIe 12V Power Monitor 1 Current
volt_3V3=$(cat $SYSFS_PATH/in3_input)       # in3_label = PCIe 3V3 Power Monitor 2 Voltage
amp_3V3=$(cat $SYSFS_PATH/curr2_input)      # curr2_label = PCIe 3V3 Power Monitor 1 Current
board_power=$(cat $SYSFS_PATH/power1_input) # power1_label = Board Power

# Append raw measures to file
# Expecting header "12V Voltage; 12V Current; 3V3 Voltage; 3V3 Current, Board Power"
echo "$timestamp;$volt_12V;$amp_12V;$volt_3V3;$amp_3V3;$board_power" >> $OUTPUT_FILE
