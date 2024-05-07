#!/bin/bash

#############
# Build AFU #
#############

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
