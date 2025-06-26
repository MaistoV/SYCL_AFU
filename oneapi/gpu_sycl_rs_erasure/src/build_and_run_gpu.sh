#!/bin/bash

# Build SYCL code for GPU
source /opt/intel/oneapi/setvars.sh > /dev/null 2>&1

# RS config
# RS=3_2
RS=6_3
# RS=$1

# Binary name
BIN=rs_erasure_gpu_$RS

# Clean up
rm -f $BIN

# Source file list
FILE_LIST=""
FILE_LIST="$FILE_LIST host.cpp"
FILE_LIST="$FILE_LIST rs_erasure_gpu.cpp "
FILE_LIST="$FILE_LIST roms/rs_rom_utils.cpp "

# Include flags
INC_FLAGS=""
INC_FLAGS="$INC_FLAGS -Iroms"
 
# Defines
DEFINE_FLAGS=""
DEFINE_FLAGS="$DEFINE_FLAGS -DMULTI_ERASURE_SIMPLE"
DEFINE_FLAGS="$DEFINE_FLAGS -DRS_$RS"
# DEFINE_FLAGS="$DEFINE_FLAGS -DDEBUG"

# Additional flags
MORE_FLAGS=""

# Build
icpx -fsycl \
    $INC_FLAGS \
    $DEFINE_FLAGS \
    $MORE_FLAGS \
    $FILE_LIST \
    -o $BIN

# Check and run
# CELL_LENGTH=$((1024))
if [ $? -eq 0 ]; then
    # ./$BIN \
    #     -l $CELL_LENGTH \
    #     -m 1 \
    #     -c 20 \
    #     ;
    bash run_gpu.sh $BIN
else
    echo "Error building!!"
    echo ""
    echo ""
fi

