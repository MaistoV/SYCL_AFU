#!/bin/bash: 
${HEM_OUT_DIR=../../../results}
OUT_FILE=$HEM_OUT_DIR/trput_$(hostname).csv
touch $OUT_FILE
echo "Writing results to $OUT_FILE"


# # mode throughput (should be higher ther lpbk)
# host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 0 # rd-wr-rd-wr
# host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 1 # rd-rd-wr-wr
# host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk --interleave 2 # rd-rd-rd-rd-wr-wr-wr-wr

# host_exerciser --clock-mhz 400 --mode trput --cls cl_1 lpbk 
# host_exerciser --clock-mhz 400 --mode trput --cls cl_2 lpbk 
# host_exerciser --clock-mhz 400 --mode trput --cls cl_4 lpbk 
# host_exerciser --clock-mhz 400 --mode trput --cls cl_8 lpbk 

