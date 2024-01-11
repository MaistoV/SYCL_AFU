#!/bin/bash

# Run for different frequencies
declare -a freq_list=( 50 100 200 400 800 )

# Create new empty file
> tmp.experiments.txt

num_runs=$1

# Loop over various lengths
for freq in "${freq_list[@]}"
do
    for (( i=1; i<=$num_runs; i++ ))
    do
        CMD="$freq"
        echo ${CMD} >> tmp.experiments.txt
    done
done

# Shuffle experiment points for randomization
shuf tmp.experiments.txt -o tmp.experiments.txt

