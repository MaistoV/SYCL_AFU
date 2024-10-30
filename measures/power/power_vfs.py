#!/bin/python

import pandas
import numpy

# Default args
input_file = "measures/power/data/power_flat_6x_RS_6_3.csv"
# output_file = "measures/power/power_vfs.csv"

# Parse args

# Read input data
data_df = pandas.read_csv(input_file, sep=";")

# Compute average
power_12V_W    = data_df["12V Voltage(mV)"].mean() * data_df["12V Current(mA)"].mean() / 1e6
power_3V3_W    = data_df["3V3 Voltage(mV)"].mean() * data_df["3V3 Current(mA)"].mean() / 1e6
board_power_W  = data_df["Board Power(mW)"].mean() / 1e3

# Debug
print("power_12V (W)    ", power_12V_W   )
print("power_3V3 (W)    ", power_3V3_W   )
print("board_power (W)  ", board_power_W )

print("power_12V (W),power_3V3 (W),board_power (W)")
print(str(power_12V_W) + "," + str(power_3V3_W) + "," + str(board_power_W))

# Append to output file