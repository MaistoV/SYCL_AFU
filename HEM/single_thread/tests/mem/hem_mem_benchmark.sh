#!/bin/bash
: ${HEM_OUT_DIR=../../../results}
OUT_FILE=$HEM_OUT_DIR/mem_$(hostname).csv
touch $OUT_FILE
echo "Writing results to $OUT_FILE"

# HEM-MEM (external memory test) 
# Test nominal values
echo "GB/s(computed), Clocks(measured)" > $OUT_FILE
for i in {1..10}
do
    # Run test
    echo "[INFO] Running $i"
    host_exerciser mem > host_exerciser_mem.log

    # Save and format results
    printf $(grep GB/s   host_exerciser_mem.log | awk '{print $2}') >> $OUT_FILE
    printf ", " >> $OUT_FILE
    printf $(grep clocks host_exerciser_mem.log | awk '{print $4}') >> $OUT_FILE
    printf "\n" >> $OUT_FILE
done
