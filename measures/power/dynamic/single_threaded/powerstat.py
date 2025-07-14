#!/bin/python3
# Description:
#   Parse CSV from powerstat log, preprocessd with the following bash commands:
#       sudo powerstat 0.5 -Rn > powerstat.log
#       cat powerstat.log | egrep -v "(Running|Power|-|Summary|Average|GeoMean|StdDev|Minimum|Maximum|These|CPU)" | sed -E "s/ +/;/g" | sed "s/;Time/Time/" | sed "s/Watts/Watts;/" > powerstat.csv

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
RS_SCHEMA_txt = [
                "RS[3:2]",
                "RS[6:3]",
        ]

# Font size
plt.rcParams.update({'font.size': 15})

# Drop this?
power_1VF_12V   = [27.4 , 27.78 ]
power_1VF_3V    = [2.7  , 2.68  ]
power_1VF_board = [42.69, 43.44 ]

##############
# Parse args #
##############

# Source data directory
data_dir = "single_threaded/data"
if len(sys.argv) >= 2:
	powerstat_csv = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 3:
	plot_dir = sys.argv[2]

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

######################
# For each RS_SCHEMA #
######################
powerstat_df = [_ for _ in range(len(RS_SCHEMA_list))]

plt.figure(figsize=[15,10])
num_rows = 1
num_cols = 2
for rs in range(0,len(RS_SCHEMA_list)):
    #############
    # Read data #
    #############
    powerstat_csv = data_dir + "/" + RS_SCHEMA_list[rs] + "/powerstat.csv"
    powerstat_df[rs] = pandas.read_csv(powerstat_csv, sep=";")
    # Clip last samples
    CLIP_SAMPLES = 100
    if rs == 0:
        powerstat_df[rs] = powerstat_df[rs][0:CLIP_SAMPLES]
    # print(powerstat_df[rs])

    # Plot
    plt.subplot(num_rows,num_cols,rs+1)
    plt.title("Raw data " + RS_SCHEMA_txt[rs])
    # Power
    plt.plot(
            powerstat_df[rs]["Time"].values,
            powerstat_df[rs]["Watts"].values,
            "-o",
            label="Watts"
            )
    # Sys
    plt.plot(
            powerstat_df[rs]["Time"].values,
            powerstat_df[rs]["Sys"].values,
            "-r+",
            label="Sys"
            )
    # User
    plt.plot(
            powerstat_df[rs]["Time"].values,
            powerstat_df[rs]["User"].values,
            "-go",
            label="User"
            )
    plt.plot(
            powerstat_df[rs]["Time"].values,
            powerstat_df[rs]["IRQ/s"].values * 0.001,
            "-kx",
            label="10k IRQ/s"
            )
    # Decorate
    plt.legend(loc='upper right')
    plt.xticks(rotation=90)
    plt.grid(axis="y")

# Save
figname = plot_dir + "/raw.png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print(figname)

################
# Process data #
################

plt.figure(figsize=[15,3])
ax = plt.subplot(num_rows,num_cols,1)
plt.xticks([])
for rs in range(0,len(RS_SCHEMA_list)):
    # Quantize in 0/1
    is_user = [0 for _ in range(len(powerstat_df[rs]["User"].values))]
    for index,row in powerstat_df[rs].iterrows():
        if powerstat_df[rs]["User"].values[index] >= 1.0:
            is_user[index] = 1

    # print(is_user)
    # print(user_df)

    # Remove spikes
    # user_df = user_df.loc[user_df["Watts"] < 50] - 50
    # print(user_df)
    # Cap to a max
    POWER_CAP = 80.
    user_df = powerstat_df[rs]
    for index,row in user_df.iterrows():
        if user_df["Watts"].values[index] > POWER_CAP:
            # print(index)
            user_df["Watts"].values[index] = user_df["Watts"].values[index-1]

    # Filter
    # TBD?

    ########
    # Plot #
    ########

    ax = plt.subplot(num_rows,num_cols,rs+1, sharey=ax)
#     ax = plt.subplot(num_rows,num_cols,rs+1)
    plt.title(RS_SCHEMA_txt[rs])
    # VF power
#     plt.axhline(y=power_1VF_12V  [rs], color='k', linestyle='-' , label="1 VF 12V (W)")
#     plt.axhline(y=power_1VF_board[rs], color='k', linestyle='--', label="Board 12V (W)")

    # Power
    plt.plot(
            user_df["Time"].values,
            user_df["Watts"].values * is_user,
        #     user_df["Watts"].values * powerstat_df[rs]["User"].values / 100.,
            "-bo",
            label="CPU power (W)"
            )
    # User
    plt.plot(
            user_df["Time"].values,
            user_df["User"].values,
            "-gP",
            label="User (%)"
            )
    # User
    plt.plot(
            powerstat_df[rs]["Time"].values,
            powerstat_df[rs]["IRQ/s"].values * 0.001,
            "-kx",
            label="10k IRQ/s"
            )
    # Sys
#     plt.plot(
#             user_df["Time"].values,
#             user_df["Sys"].values,
#             "-ro",
#             label="Sys"
#             )
    # Decorate
    plt.xlabel("Time")
    # plt.ylabel("Watts")
    plt.xticks([])
    # plt.xticks(rotation=90)
    plt.grid(axis="y")
    if rs == 0:
        plt.legend(loc='upper right')

# Save
figname = plot_dir + "/power.png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print(figname)
