#!/bin/python

# Imports
import os
import pandas
import matplotlib.pyplot as plt
# import plot_common as common

# Parse args
input_file = "measures/power/power_vfs.csv"
plot_dir = "measures/power/plots"

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

# Read data
data_df = pandas.read_csv(input_file, sep=";")
pandas.to_numeric(data_df["numVFs"])
print(data_df)

# RS[3:2]
rs_3_2_df = data_df.loc[
                (data_df["K:P"] == "3:2") &
                (data_df["Bitstream"] == "flat")
            ]
print(rs_3_2_df)

# RS[6:3]
rs_6_3_df = data_df.loc[
                (data_df["K:P"] == "6:3") &
                (data_df["Bitstream"] == "flat")
            ]
print(rs_6_3_df)


# Common
range_numVFs=range(data_df["numVFs"].min(), data_df["numVFs"].max() +1)
MARKER_SIZE = 10
markers = ["o", "d", "X", "P"]
RS_SCHEMA_list = ["3_2", "6_3" ]
RS_SCHEMA_txt  = ["3:2", "6:3" ]
RS_color = ["g", "b"]
ASP_color = ["g", "b"]
RS_6_3 = 1
RS_3_2 = 0

# Figure
plt.figure("VF power scaling", figsize=[15,5])

# Loop over RS_SCHEMAs
ax = plt.subplot(1,2,1)
plt.xticks([])
for rs in range(0,len(RS_SCHEMA_list)):
    # Subplot
    ax = plt.subplot(1,2,rs+1, sharey=ax)
    plt.title("RS[" + RS_SCHEMA_txt[rs] + "]")

    # Select data
    print_df = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "flat")
                ]
    # Plot
    plt.plot(
            print_df["numVFs"],
            print_df["board_power (W)"],
            "-o",
            markersize=MARKER_SIZE,
            color=RS_color[rs],
            label="SYCL AFU Board Power"
        )
    plt.plot(
            print_df["numVFs"],
            print_df["power_12V (W)"],
            "-d",
            markersize=MARKER_SIZE,
            color=RS_color[rs],
            label="SYCL AFU 12V"
        )
    # plt.plot(
    #         print_df["numVFs"],
    #         print_df["power_3V3 (W)"],
    #         "-*",
    #         color=RS_color[rs],
    #         label="RS[" + RS_SCHEMA_txt[rs] + "] 3V3"
    #     )

    # Select data
    print_df = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "asp")
                ]
    plt.plot(
            print_df["numVFs"],
            print_df["board_power (W)"],
            "o",
            markersize=MARKER_SIZE,
            color="r",
            label="ASP Board Power"
        )
    plt.plot(
            print_df["numVFs"],
            print_df["power_12V (W)"],
            "d",
            markersize=MARKER_SIZE,
            color="r",
            label="ASP 12V"
        )
    # Print ASP projections
    numVFs = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "flat")
                ]["numVFs"]
    asp_projection = numVFs * print_df["power_12V (W)"].values
    plt.plot(
            numVFs,
            asp_projection,
            "--o",
            markersize=MARKER_SIZE / 2,
            markerfacecolor='none',
            color="r",
        )
    asp_projection = numVFs * print_df["board_power (W)"].values
    plt.plot(
            numVFs,
            asp_projection,
            "--o",
            markersize=MARKER_SIZE / 2,
            markerfacecolor='none',
            color="r",
        )

    # Decorate
    plt.yscale('log')
    plt.xlabel("Number of VFs")
    plt.ylabel("Power (W)")
    plt.grid(visible=True, axis="y", which="both")
    plt.legend()
    plt.xticks( data_df.loc[
                        (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                        (data_df["Bitstream"] == "flat")
                    ]["numVFs"]
        )

# Save figure
figname = plot_dir + "/vf_power.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")

# Figure
plt.figure("VF power scaling (board-only)", figsize=[5,5])

# Loop over RS_SCHEMAs
# ax = plt.subplot(1,2,1)
# plt.xticks([])
# Reduce marker size here
MARKER_SIZE = MARKER_SIZE - 2
for rs in range(0,len(RS_SCHEMA_list)):
    rs_name = "RS[" + RS_SCHEMA_txt[rs] + "]"

    # Select data
    column = "board_power (W)"
    # column = "power_12V (W)"
    print_df = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "flat")
                ]
    # Plot
    plt.plot(
            print_df["numVFs"],
            print_df[column],
            "-" + markers[2*rs],
            markersize=MARKER_SIZE,
            color=RS_color[rs],
            label= rs_name + " SYCL AFU"
        )

    # Select data
    print_df = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "asp")
                ]
    plt.plot(
            print_df["numVFs"],
            print_df[column],
            markers[2*rs + 1],
            markersize=MARKER_SIZE,
            color="r",
            label= rs_name + " ASP"
        )
    # Print ASP projections
    numVFs = data_df.loc[
                    (data_df["K:P"] == RS_SCHEMA_txt[rs]) &
                    (data_df["Bitstream"] == "flat")
                ]["numVFs"]
    asp_projection = numVFs * print_df[column].values
    plt.plot(
            numVFs,
            asp_projection,
            "--" + markers[2*rs + 1],
            markersize=MARKER_SIZE / 2,
            markerfacecolor='none',
            color="r",
        )

    # Decorate
    plt.xlabel("Number of VFs")
    plt.ylabel("Power (W)")
    plt.yscale('log')
    plt.grid(visible=True, axis="y", which="both")
    plt.legend()
    plt.xticks( range_numVFs )

# Save figure
figname = plot_dir + "/vf_power_board.png"
print(figname)
plt.savefig(figname, dpi=400, bbox_inches="tight")
