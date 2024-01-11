#!/bin/bash
: ${OUT_DIR=$PWD/../../../results}
mkdir -p $OUT_DIR
OUT_FILE=$OUT_DIR/multithreading_trput_$(hostname).csv
touch $OUT_FILE
echo "Writing results to $OUT_FILE"

# Generate experiments
source ../../../single_thread/tests/freq/gen_experiments_freq.sh 10
readarray -t experiment_list < tmp.experiments.txt 

vf_list=($PCIE_HEM_LPBK $PCIE_HEM_LPBK_bis)
for vf in "${vf_list[@]}"
do
    echo "Creating directory $vf"
    mkdir -p $vf
done

# Output file
echo "MHz, VF, GB/s(computed), Clocks(measured)" > $OUT_FILE
i=0
for freq in "${experiment_list[@]}"
do  
    # Loop over VFs
    for vf in "${vf_list[@]}"
    do
      i=$(($i+1))
      echo "Running test $i/$((${#experiment_list[@]} * ${#vf_list[@]}))"

      # Run from different directory to keep the logs
      cd $vf 
      host_exerciser --pci-address $vf lpbk &
      cd ..
    done

    # Wait both children
    wait

    # Save and format results
    for vf in "${vf_list[@]}"
    do
      cd $vf 
      
      printf "$freq, $vf, " >> $OUT_FILE
      printf $(grep GB/s   host_exerciser_lpbk.log | awk '{print $2}') >> $OUT_FILE
      printf ", " >> $OUT_FILE
      printf $(grep clocks host_exerciser_lpbk.log | awk '{print $4}') >> $OUT_FILE
      printf "\n" >> $OUT_FILE

      cd ..
    done
done

