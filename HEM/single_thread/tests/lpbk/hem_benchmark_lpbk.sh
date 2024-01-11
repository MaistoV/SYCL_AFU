: ${OUT_DIR=../../../results}
OUT_FILE=$OUT_DIR/lpbk_$(hostname).csv
touch $OUT_FILE
echo "Writing results to $OUT_FILE"

# Nominal 400 MHz frequency input    
MODES_ARRAY=( lpbk read write trput )
VF_ARRAY=( $PCIE_HEM_LPBK $PCIE_HEM_LPBK_bis )

echo "S:B:D:F, Mode, GB/s(computed), Clocks(measured)" > $OUT_FILE
for vf in "${VF_ARRAY[@]}"
do
    for mode in "${MODES_ARRAY[@]}"
    do
        FLAGS=" --clock-mhz 400 --pci-address $vf --mode $mode"
        printf "$vf, $mode, " >> $OUT_FILE
        host_exerciser $FLAGS lpbk 
        printf $(grep Bandwidth host_exerciser_lpbk.log | awk '{print $2}')  >> $OUT_FILE
        printf ", "                                                          >> $OUT_FILE
        printf $(grep clocks    host_exerciser_lpbk.log | awk '{print $4}')  >> $OUT_FILE
        printf "\n"                                                          >> $OUT_FILE
    done
done