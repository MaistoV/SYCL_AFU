#!/bin/bash

# Parse arg1 to set variables
MAKE_BUILD_TARGET=oneapi_$1
MAKE_TEST_TARGET=test_$1
NUM_RUNS=$2
MAX_DECODE=$3
MULTI_THREAD=$4

# Default
MAKE_SETUP_TARGET=help # "make help" does nothing
DECODE_ISAL=0 # Run on FPGA
EXPERIMENT_PROFILE=${EXPERIMENT_PROFILE} # From environment

case $1 in

  "isal")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_ISA_L
    DECODE_ISAL=1
    MAKE_BUILD_TARGET=afu_host
    ;;

  "asp_fpga")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_ASP
    MAKE_SETUP_TARGET=aocl_aocx_initialize
    # Append ASP_ZERO_COPY
    if [[ $ASP_ZERO_COPY == 1 ]]; then
        export MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_OUTPUT_DIR}_ASP_ZERO_COPY
    fi
    ;;

  "asp_fpga_sim")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_ASP_SIM 
    # Override
    MEASURE_NUM_REPS=1
    MEASURE_MAX_DECODE=1
    EXPERIMENT_PROFILE="SIM"
    # Append ASP_ZERO_COPY
    if [[ $ASP_ZERO_COPY == 1 ]]; then
        export MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_OUTPUT_DIR}_ASP_ZERO_COPY
    fi 
    ;;

  "sycl_afu")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_AFU
    MAKE_BUILD_TARGET=afu_host
    MAKE_SETUP_TARGET=gbs_configure
    ;;

  # For debug
  "asp_plain_c")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;
  "ip_plain_c")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;

  *)
    echo "Arg1 must be in {isal, asp_fpga, asp_fpga_sim, sycl_afu}" >&2
    exit -1
    ;;
esac

# Append pid for MULTI_THREADing
if [ ${MULTI_THREAD} -eq 1 ]; then
  MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_OUTPUT_DIR}_$$
fi

# Lauch subscript
source ${MEASURE_LATENCY_DIR}/scripts/measure_latency.sh 	\
	${DECODE_ISAL} 							      \
	${NUM_RUNS}          					    \
	${MEASURE_LATENCY_OUTPUT_DIR}     \
  ${MAX_DECODE}                     \
  ${MAKE_TEST_TARGET}               \
  ${MAKE_BUILD_TARGET}              \
  ${MAKE_SETUP_TARGET}              \
  ${EXPERIMENT_PROFILE}

# NOTE: this is a dirty workaround
# For plain_c runs, rename output files
if [[ $1 == *"plain_c"* ]]; then
  for file in ${MEASURE_LATENCY_OUTPUT_DIR}/*; do
      echo $file > tmp
      sed -i "s/SYCL_ASP/PLAIN_C/g" tmp
      cat tmp | xargs mv $file
  done
  rm tmp
fi
