#!/bin/python3
# Description:
#   Parse CSV from top log, preprocessd with the following bash commands:
    #   top -b -n 60 -d 1 | grep sycl_rs_erasure > top.log; \
    #   echo ";PID;USER;PR;NI;VIRT;RES;SHR;S;%CPU;%MEM;TIME+;COMMAND" > top.csv; \
    #   sed -E "s/ +/;/g" top.log >> top.csv; \
    #   sed -i "s/,/./g" top.csv

import sys
import os
import pandas
import matplotlib.pyplot as plt
# import glob
# import numpy

#############
# Constants #
#############
RS_SCHEMA_list = [
                "RS_3_2",
                "RS_6_3",
        ]

# Drop this?
# power_1VF_12V   = [27.4 , 27.78 ]
# power_1VF_3V    = [2.7  , 2.68  ]
# power_1VF_board = [42.69, 43.44 ]

##############
# Parse args #
##############

# Source data directory
data_dir = "./data"
if len(sys.argv) >= 2:
	top_csv = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 3:
	plot_dir = sys.argv[2]

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

######################
# For each RS_SCHEMA #
######################
top_df = [_ for _ in range(len(RS_SCHEMA_list))]

plt.figure(figsize=[15,10])
num_rows = 2
num_cols = 1
for rs in range(0,len(RS_SCHEMA_list)):
    #############
    # Read data #
    #############
    top_csv = data_dir + "/" + RS_SCHEMA_list[rs] + "/top.csv"
    top_df[rs] = pandas.read_csv(top_csv, sep=";")
    # Clip last samples
    # top_df[rs] = top_df[rs][0:70]
    print(top_df[rs])
    print(top_csv)

    # Plot
    plt.subplot(num_rows,num_cols,rs+1)
    plt.title("Raw data " + RS_SCHEMA_list[rs])
    # Power
    plt.plot(
            top_df[rs].index,
            top_df[rs]["%CPU"].values.astype(float),
            "-o",
            )
    # Sys
#     plt.plot(
#             top_df[rs]["Time"].values,
#             top_df[rs]["Sys"].values,
#             "-ro",
#             label="Sys"
#             )
#     plt.plot(
#             top_df[rs]["Time"].values,
#             top_df[rs]["IRQ/s"].values * 0.001,
#             "-ko",
#             label="10k IRQ/s"
#             )
    # Decorate
    plt.xticks(rotation=90)
    plt.grid(True, axis="y")
    # Decorate
    plt.xlabel("Time")
    plt.ylabel("CPU %")
    plt.xticks([])
    # plt.xticks(rotation=90)
    if rs == 0:
        plt.legend()


# Save
figname = plot_dir + "/top.raw.png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print(figname)

################
# Process data #
################

plt.figure(figsize=[15,10])
for rs in range(0,len(RS_SCHEMA_list)):
    # Remove spikes
    # user_df = user_df.loc[user_df["Watts"] < 50] - 50
    # print(user_df)
    # Cap to a max
    # POWER_CAP = 50.
    # user_df = top_df[rs]
    # for index,row in user_df.iterrows():
    #     if user_df["Watts"].values[index] > POWER_CAP:
    #         # print(index)
    #         user_df["Watts"].values[index] = user_df["Watts"].values[index-1]

    # Filter
    # TBD?

    ########
    # Plot #
    ########

    plt.subplot(num_rows,num_cols,rs+1)

    # Power
    df = top_df[rs].loc[top_df[rs]["PID"] == min(top_df[rs]["PID"])]
    plt.plot(
            df.index,
            df["%CPU"].values.astype(float),
            "-o",
            label="ISA-L"
            )
    df = top_df[rs].loc[top_df[rs]["PID"] == max(top_df[rs]["PID"])]
    df = top_df[rs].loc[top_df[rs]["PID"] == max(top_df[rs]["PID"])]
    plt.plot(
            df.index,
            df["%CPU"].values.astype(float),
            "-o",
            label="AFU"
            )

    # Decorate
    plt.xlabel("Time")
    # plt.ylabel("Watts")
    plt.xticks([])
    # plt.xticks(rotation=90)
    plt.grid(True, axis="y")
    if rs == 0:
        plt.legend()

# Save
figname = plot_dir + "/top.power.png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print(figname)
