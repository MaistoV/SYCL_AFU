#!/bin/bash
: ${HEM_OUT_DIR=../../../results}
OUT_FILE=$HEM_OUT_DIR/freq_$(hostname).csv
touch $OUT_FILE
echo "Writing results to $OUT_FILE"

# Generate experiments
source gen_experiments_freq.sh 10
readarray -t experiment_list < tmp.experiments.txt 

afu_list=(lpbk mem)

# Output file
echo "AFU, MHz, GB/s(computed), Clocks(measured)" > $OUT_FILE
i=0
for afu in "${afu_list[@]}"
do
    for freq in "${experiment_list[@]}"
    do  
      i=$(($i+1))
      echo "Running test $i/$((${#experiment_list[@]} * ${#afu_list[@]}))"
      host_exerciser --clock-mhz $freq $afu > host_exerciser_$afu.log
    
      # Save and format results
      printf "$afu, $freq, " >> $OUT_FILE
      printf $(grep GB/s   host_exerciser_$afu.log | awk '{print $2}') >> $OUT_FILE
      printf ", " >> $OUT_FILE
      printf $(grep clocks host_exerciser_$afu.log | awk '{print $4}') >> $OUT_FILE
      printf "\n" >> $OUT_FILE
    done
done
