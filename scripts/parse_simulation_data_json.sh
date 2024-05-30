#!/bin/bash

# TARGET_DIR=oneapi/sycl_rs_erasure/build_asp_RS_3_2_ofs_nc220_usm_ASP_ZERO_COPY/sycl_rs_erasure_RS_3_2.fpga_sim.prj/reports/resources

TARGET_FILE=$1/reports/resources/simulation_stats_data.js
OUTPUT_FILE=$2
CELL_LENGTH=$3

latency_array=(
        $(
            cat $TARGET_FILE                     | \
            sed -E "s/.+=//g"                    | \
            sed "s/;//g"                         | \
            jq -r  ".functions[0].data.latency"  | \
            sed "s/,/ /g"
        )
    )

# latency_min=${latency_array[0]}
# latency_max=${latency_array[1]}
# latency_avg=${latency_array[2]}
# echo latency_min=$latency_min
# echo latency_max=$latency_max
# echo latency_avg=$latency_avg

# If file does not exist already
if [ ! -e $OUTPUT_FILE ]; then
    # Create header
    echo "Cell length,min,max,avg" > $OUTPUT_FILE
fi
# Append result
echo ${CELL_LENGTH},${latency_array[0]},${latency_array[1]},${latency_array[2]} >> $OUTPUT_FILE
