#!/bin/bash

# Parse arg1 to set variables
MAKE_TEST_TARGET=test_$1
MAKE_BUILD_TARGET=oneapi_$1
DECODE_ISAL=0
NUM_RUNS=$2
case $1 in

  "isal")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DIR}/data/data_ISA_L
    DECODE_ISAL=1
    ;;

  "asp_fpga")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DIR}/data/data_ASP
    # make aocl_aocx_initalize
    # make opae.io_bind_one
    ;;

  "sycl_afu")
    MEASURE_LATENCY_OUTPUT_DIR=${MEASURE_LATENCY_DIR}/data/data_SYCL_AFU
    make gbs_config
    make opae.io_bind_one
    ;;

  *)
    echo "Arg1 must be in {isal, asp_fpga, sycl_afu}" >&2
    ;;
esac

# Lauch subscript
source ${MEASURE_LATENCY_DIR}/measure_latency.sh 	\
	${DECODE_ISAL} 							        \
	${NUM_RUNS}          					        \
	${MEASURE_LATENCY_OUTPUT_DIR}