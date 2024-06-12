#!/bin/bash

MAKE_TARGET=$1
NUM_THREADS=$2

# Launch targets in background
for (( i=0 i<${NUM_THREADS}; i++ )); do
    make -C ${ROOT_DIR} ${MAKE_TARGET} \
        MULTI_THREAD=1 \
        SBDF=${PAC_PCIE_SBD}.$((${FIRST_AFU_VF} + $i)) \
        &
done

# Wait for all children
wait