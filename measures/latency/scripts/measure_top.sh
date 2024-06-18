#!/bin/bash

# Parse args to set variables
MAKE_TEST_TARGET=test_$1
NUM_REPS=$2
MAX_DECODE=$3
MULTI_THREAD=$4
SBDF=$5
THREAD_INDEX=$6

# Default
DECODE_ISAL=0 # Run on FPGA
EXPERIMENT_PROFILE=${EXPERIMENT_PROFILE} # From environment

case $1 in

  "isal")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_ISA_L
    DECODE_ISAL=1
    ;;

  "asp_fpga")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_ASP
    # Append ASP_ZERO_COPY
    if [[ $ASP_ZERO_COPY == 1 ]]; then
        export OUTPUT_DIR=${OUTPUT_DIR}_ASP_ZERO_COPY
    fi
    ;;

  "asp_fpga_sim")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_ASP_SIM 
    # Override
    NUM_REPS=1
    MAX_DECODE=3
    EXPERIMENT_PROFILE="SIM"
    # Append ASP_ZERO_COPY
    if [[ $ASP_ZERO_COPY == 1 ]]; then
        export OUTPUT_DIR=${OUTPUT_DIR}_ASP_ZERO_COPY
    fi 
    ;;

  "sycl_afu")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_AFU
    ;;

  # For debug
  "asp_plain_c")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;
  "ip_plain_c")
    OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;

  *)
    echo "Arg1 $1 must be in {isal, asp_fpga, asp_fpga_sim, sycl_afu}" >&2
    exit -1
    ;;
esac

# Append pid for multi_threading
if [ ${MULTI_THREAD} -eq 1 ]; then
  OUTPUT_DIR=${OUTPUT_DIR}_${THREAD_INDEX}
fi

# Lauch subscript
source ${MEASURE_LATENCY_DIR}/scripts/measure.sh 	\
	${DECODE_ISAL} 							      \
	${NUM_REPS}          					    \
	${OUTPUT_DIR}                     \
  ${MAX_DECODE}                     \
  ${MAKE_TEST_TARGET}               \
  ${EXPERIMENT_PROFILE}             \
  ${SBDF}

###################
# Post-processing #
###################

# For plain_c runs, rename output files
if [[ $1 == *"plain_c"* ]]; then
  for file in ${OUTPUT_DIR}/*; do
      echo $file > tmp
      sed -i "s/SYCL_ASP/PLAIN_C/g" tmp
      cat tmp | xargs mv $file
  done
  rm tmp
fi
