#!/bin/bash

#############
# Setup ASE #
#############
# Removing old directory, if any
rm -rf ${AFU_ASE_DIR} 

cd ${AFU_FLOW_DIR}

# Setup optional flags
# Verbosity
if [ "$VERBOSE" = "1" ]; then
    OPT_FLAGS="--ase-verbose"
fi
# Simuation mode
if [ "$ASE_MODE" != "" ]; then
    OPT_FLAGS="${OPT_FLAGS} --ase-mode ${ASE_MODE}"
fi

# Launch script
afu_sim_setup                       \
    --sources ${AFU_SOURCE_LIST}    \
    -t QUESTA                       \
    ${OPT_FLAGS}                    \
    ${AFU_ASE_DIR}
# Check for exit code
if [ $? -ne 0 ]; then
    echo '[ERROR] Could not setup synthesis directory.' ; exit 1;
fi

cd ${AFU_ASE_DIR}

# # Generate vsim debug info
# if [ $DEBUG_VSIM -eq 1 ]; then
#     export MENT_VSIM_OPT="-voptargs=\"-debugdb\" -debugdb"
# fi

echo "[INFO] Start compilation of full AFU bitstream..."
echo "[INFO] Using PR-tree in $(basename ${OPAE_PLATFORM_ROOT}) ..."

make
make sim

# NOTE: now in another shell source afu_ase_sw.sh

echo "You can now inspect the waves with:"
echo "  make ase_waves"