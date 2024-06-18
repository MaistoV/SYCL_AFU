#!/bin/bash

MEASURE_TARGET=$1
NUM_THREADS=$2

# declare -a PIDs=

# Always multi-threading in here
MULTI_THREAD=1

# Launch targets in background
for (( i=0; i<${NUM_THREADS}; i++ )); do
	THREAD_INDEX=$i
    # TODO: update for #NUM_THREADS >= 8, since VF index would wrap around to SSSS:BB:{DD+1}.0..
    SBDF=${PAC_PCIE_SBD}.$((${FIRST_AFU_VF} + $i)) \
    # Call single-thread script
    ${MEASURE_LATENCY_DIR}/scripts/measure_top.sh \
		${MEASURE_TARGET}       \
		${MEASURE_NUM_REPS} 	\
		${MEASURE_MAX_DECODE} 	\
		${MULTI_THREAD}			\
		${SBDF}					\
		${THREAD_INDEX} &
done

# Wait for all children
wait
