#!/bin/bash

#######################
# Setup AFU synthesis #
#######################

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

echo "[INFO] Ready to start compilation of AFU GBS"
echo "[INFO] Using PR-tree in $(basename $OPAE_PLATFORM_ROOT)"
