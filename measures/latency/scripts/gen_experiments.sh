#!/bin/bash

# Run for different cell length
# NOTE: must be integer multiple of 64 bytes
declare -a len_list=( 
                        64 128 256 512 
                        1024                $((2*1024))         $((4*1024))         $((8*1024))         $((16*1024)) 
                        $((32*1024))        $((64*1024))        $((128*1024))       $((256*1024))       $((512*1024)) 
                        $((1024*1024))      $((2*1024*1024))    $((4*1024*1024))    $((8*1024*1024))    $((16*1024*1024))    $((32*1024*1024))   $((64*1024*1024))
                         # Error creating shared memory buffer (input): no memory
                        $((128*1024*1024))
                        #$((256*1024*1024))  $((512*1024*1024)) # $((1024*1024*1024))  # too large for 6:3
                    )

num_runs=$1
filename=$2

# Create new empty file
rm $filename
touch $filename

# Loop over various lengths
for len in "${len_list[@]}"
do
    for (( i=1; i<=$num_runs; i++ ))
    do
        CMD="$len"
        echo ${CMD} >> $filename
    done
done

# Shuffle experiment points
shuf $filename -o $filename