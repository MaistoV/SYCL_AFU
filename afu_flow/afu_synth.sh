#!/bin/bash

#############
# Build AFU #
#############

# Removing old directory, if any
rm -rf ${AFU_SYNTH_DIR} 

cd ${AFU_FLOW_DIR}

# Launch script
afu_synth_setup                     \
    --sources ${AFU_SOURCE_LIST}    \
    ${AFU_SYNTH_DIR} 
# Check for exit code
if [ $? -ne 0 ]; then
    echo '[ERROR] Could not setup synthesis directory.' ; exit 1;
fi

cd ${AFU_SYNTH_DIR} 

echo "[INFO] Start compilation of full AFU bitstream..."
echo "[INFO] Using PR-tree in $(basename $OPAE_PLATFORM_ROOT) ..."
$OPAE_PLATFORM_ROOT/bin/afu_synth

# Check for exit code
if [ $? -ne 0 ]; then
    echo '[ERROR] Compilation failed.' ; exit 1;
fi
# Check for output file
if [ ! -e *.gbs ]; then
    echo '[ERROR] Cannot find GBS.' ; exit 1;
fi
