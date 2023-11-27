# #!/bin/bash

: ${OUT_FILE=hem_benchmark_$(hostname).csv}
# GREP_CMD="grep GB/s | awk '{print $2}' >> $OUT_FILE"

# HEM-LB (loop-back) -> AFU from/to host at full bandwidth
# declare -a freq_list=( 50 100 200 300 400 500 600 700 800 900 )
declare -a freq_list=( 50 )
echo "MHz, iteration" > $OUT_FILE
for freq in "${freq_list[@]}"
do
    for i in 1..10
    do
        printf "$freq, $i, " >> $OUT_FILE
        sudo host_exerciser --clock-mhz $freq \
        | grep GB/s | awk '{print $2}' >> $OUT_FILE
    done
done

# # mode throughput (should be higher ther lpbk)
# sudo host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 0 # rd-wr-rd-wr
# sudo host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 1 # rd-rd-wr-wr
# sudo host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 2 # rd-rd-rd-rd-wr-wr-wr-wr


# #Traffic Generator AFU Test Application
# TBD
# mem_tg tg_test

# HEM-MEM
# TDB
# sudo host_exerciser --clock-mhz 400 mem


# HE-HSSI
# don't care
