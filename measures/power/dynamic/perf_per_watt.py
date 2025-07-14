#!/bin/python3
# Description: Self-contained script, plotting modeled performance per Watt

import matplotlib.pyplot as plt

###############################
# Instantaneous power (Watts) #
###############################
# Core (equivalently, per thread)
#               3:2     , 6:3
# ISA-L     :   60.5    , 61.43
# Near-idle :   42.64   , 42.57
# SYCL AFU SP, max VFs
#               3:2 (13 VFs)  , 6:3 (6 VFs)
# board     :   74.25         , 70.53
# 12 V      :   40.37         , 38.9
# Cumulatively, num thread = max VFs
# NOTE: these power figures would melt the CPU, we can't just use linear scaling
# ISA-L : Ptot = Psingle-thread x num threads
# VFs   : Ptot = (Near-idle CPU x num threads) + P[maxVFs]
#                           3:2     , 6:3
# ISA-L                 :   786.5   , 368.58
# board + Near-idle core:   629,57  , 325.95
# 12 V  + Near-idle core:   594,69  , 294.32

#############
# Constants #
#############
plot_dir = "./output_plots"

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

################
# Embedde Data #
################

power = [
        # 3:2   , 6:3
        [786.50 , 368.58], # ISA-L
        [629.57 , 325.95], # board + idle
        [594.69 , 294.32], # 12 V  + idle
    ]

plt.figure(figsize=[15,2])
num_rows = 1
num_cols = 2
# ax = plt.subplot(num_rows,num_cols,1)
# for rs in range(0,len(RS_SCHEMA_list)):
#     # Figure
#     ax = plt.subplot(num_rows,num_cols,1, sharey=ax)
#     plt.title(RS_SCHEMA_txt[rs])

# Bar plot
plt.bar(
    RS_SCHEMA_txt,
    height=power[0],#[rs],
    # width=2,
    # hatch=patterns[model],
    # color=common.color_array[model],
    bottom=0,
)

# Decorate
plt.grid(True)

# Save
figname = plot_dir + "/perf_per_watt.png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print(figname)
