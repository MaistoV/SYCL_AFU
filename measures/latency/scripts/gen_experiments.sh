#!/bin/bash
###############################################
# Parameters
# $1: Number of repetitions/runs
# $2: Output file name
# $3: Profile, in {SIM, QUICK, COMPLETE}
###############################################

num_runs=$1
filename=$2
profile=$3

# Run for different cell lengths
# NOTE: Values must be integer multiple of 64 bytes
case $profile in
    "QUICK")
        declare -a len_list=( 
                1024                $((8*1024))
                $((32*1024))        $((128*1024))       $((512*1024)) 
                $((1024*1024))      $((2*1024*1024))    $((81024*1024))    $((16*1024*1024))
            )        
        ;;
    "SIM")
        declare -a len_list=( 
                64 128 256 512 1024
            )
        ;;
    "COMPLETE")
        # Complete test
        declare -a len_list=( 
                1024                $((2*1024))         $((4*1024))         $((8*1024))         $((16*1024)) 
                $((32*1024))        $((64*1024))        $((128*1024))       $((256*1024))       $((512*1024)) 
                $((1024*1024))      $((2*1024*1024))    $((4*1024*1024))    $((81024*1024))    $((16*1024*1024))
                $((32*1024*1024))   $((64*1024*1024))
            )
        ;;
esac

# Create new empty file
rm -f $filename
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