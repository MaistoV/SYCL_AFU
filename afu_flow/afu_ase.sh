#!/bin/bash

#############
# Setup ASE #
#############
# Removing old directory, if any
rm -rf ${AFU_ASE_DIR} 

SYCL_AFU_DIR=${AFU_FLOW_DIR}/afus/sycl_afu/hw/rtl/

echo $SYCL_AFU_DIR
# In case of imported IP, prepare Questa file list from kernel_system.qip, using the original ASP flow
if [ -d ${SYCL_AFU_DIR}/${SYCL_IP_NAME}_report.prj ]; then
    # Move to where the kernel_system.qip file is
    cd ${SYCL_AFU_DIR}/${SYCL_IP_NAME}_report.prj
    echo "[INFO] Launching ase-sim-compile.sh"
    # NOTE: this script is the same as for n6001
    # NOTE: this script strictly relies on oneapi-asp's tag: ofs-2023.3-2
    source ${OFS_ASP_ROOT}/hardware/ofs_nc220/build/scripts/ase-sim-compile.sh
    # Remove output
    rm -rf ../../../../../../fpga.bin simulation.tar.gz
fi

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
    echo "[ERROR] Could not setup synthesis directory." ; exit 1;
fi

cd ${AFU_ASE_DIR}

# # Generate vsim debug info
# if [ $DEBUG_VSIM -eq 1 ]; then
#     export MENT_VSIM_OPT="-voptargs=\"-debugdb\" -debugdb"
# fi

echo "[INFO] Start compilation of full AFU bitstream..."
echo "[INFO] Using PR-tree in $(basename ${OPAE_PLATFORM_ROOT}) ..."

# Launch simulation
make
make sim

echo "[INFO] On success, you can inspect the waves with:"
echo "[INFO]   make ase_waves"
