#!/bin/bash

MAKE_TARGET=$1
NUM_THREADS=$2
# Launch targets in background
for (( i=1; i<=${NUM_THREADS}; i++ )); do
    MULTI_THREAD=1           \
    make -C ${ROOT_DIR} ${MAKE_TARGET} &
done