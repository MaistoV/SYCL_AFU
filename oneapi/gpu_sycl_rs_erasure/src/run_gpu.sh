#!/bin/bash

# Binary to run
BIN=$1

# Number of repetitions
NUM_REPS=10

# Number of reconstructions
NUM_RECONSTRUCTIONS=10

# Cell lengths
lengths=(
        # 64
        1024
        $((1024*2))
        $((1024*4))
        $((1024*8))
        $((1024*16))
        $((1024*32))
        $((1024*64))
        $((1024*128))
        $((1024*256))
        $((1024*512))
        $((1024*1024))
        $((1024*1024*2))
        $((1024*1024*4))
        $((1024*1024*8))
        $((1024*1024*16))
    )

DATA_DIR="data/"
mkdir -p $DATA_DIR

for (( i=1; i<=$NUM_REPS; i++ )); do
    # Loop over lengths
    for l in ${lengths[*]}; do
        ./$BIN \
            -l $l \
            -m 1 \
            -e 1 \
            -d 0 \
            -c $NUM_RECONSTRUCTIONS \
            -o $DATA_DIR \
            ;
    done
done