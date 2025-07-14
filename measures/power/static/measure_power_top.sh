#!/bin/bash

# Output file
OUTPUT_FILE=power_default.csv
if [[ "$1" != "" ]]; then
    OUTPUT_FILE=$1
fi

mkdir -p $(dirname $OUTPUT_FILE)

# Sleep time between sysfs reads
# If much larger than the file access time, this could approximate the sampling period
SLEEP_TIME=0.2

# This changes across boots
BASE_SYSFS_PATH=/sys/bus/pci/devices/${PAC_PCIE_SBD}.0/fpga_region/region0/dfl-fme.0
SYSFS_PATH=$(find ${BASE_SYSFS_PATH} -name n6000bmc-hwmon.2.auto)/hwmon/hwmon3

# Create if does not exist
if [ ! -e $OUTPUT_FILE ]; then
    # NOTE: BMC output is in mW, not compensated by hwmon driver, which should output uW, see https://www.kernel.org/doc/Documentation/hwmon/sysfs-interface
    echo "Time(s);12V Voltage(mV);12V Current(mA);3V3 Voltage(mV);3V3 Current(mA);Board Power(mW)" > $OUTPUT_FILE
fi

# Launch a busy loop, polling with subscript
# cnt=0
# while (true) do
#     cnt=$((cnt+1))
MAX_SAMPES=100
for ((cnt = 0 ; cnt < $MAX_SAMPES ; cnt++ )); do
    echo "Collecting sample #$cnt, kill to stop"
    # Launch measure
    ${ROOT_DIR}/measures/power/measure_power.sh \
        $SYSFS_PATH \
        $OUTPUT_FILE
    # Sleep between samples
    sleep $SLEEP_TIME
done

echo "Data available at $OUTPUT_FILE"