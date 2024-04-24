#!/bin/bash

# Parse arg1 to set variables
MAKE_TEST_TARGET=test_$1
MAKE_BUILD_TARGET=oneapi_$1
DECODE_ISAL=0
NUM_RUNS=$2
MAX_DECODE=$3

case $1 in

  "isal")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_ISA_L
    DECODE_ISAL=1
    ;;

  "asp_fpga")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_ASP
    # NOTE: this requires sudo, hence will stall the prompt by default
    make aocl_aocx_initalize
    ;;

  "sycl_afu")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_SYCL_AFU
    # NOTE: this requires sudo, hence will stall the prompt by default
    make gbs_config
    make opae.io_bind_one
    ;;

  # For debug
  "asp_plain_c")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;
  "ip_plain_c")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DATA_DIR}/data_PLAIN_C
    ;;

  *)
    echo "Arg1 must be in {isal, asp_fpga, sycl_afu}" >&2
    exit -1
    ;;
esac

# Lauch subscript
source ${MEASURE_LATENCY_DIR}/scripts/measure_latency.sh 	\
	${DECODE_ISAL} 							      \
	${NUM_RUNS}          					    \
	${MEASURE_LATENCY_OUTPUT_DIR}     \
  ${MAX_DECODE}

# NOTE: this is a dirty workaround
# For plain_c runs, rename output files
if [[ $1 == *"plain_c"* ]]; then
  for file in ${MEASURE_LATENCY_OUTPUT_DIR}/*; do
      echo $file > tmp
      sed -i "s/SYCL_ASP/PLAIN_C/g" tmp
      cat tmp | xargs mv $file
  done
fi
rm tmp
