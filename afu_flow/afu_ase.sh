#!/bin/bash

#############
# Setup ASE #
#############
# Removing old directory, if any
rm -rf ${AFU_ASE_DIR} 

if [[ ${AFU_NAME} == *sycl* ]]; then
    # In case of SYCL-imported IP, prepare Questa file list from kernel_system.qip, using the original ASP flow
    SYCL_AFU_DIR=${AFU_FLOW_DIR}/afus/sycl_afu/hw/rtl/
    if [ -d ${SYCL_AFU_DIR}/${SYCL_IP_NAME}_report.prj ]; then
        # Move to where the kernel_system.qip file is
        cd ${SYCL_AFU_DIR}/${SYCL_IP_NAME}_report.prj
        echo "[INFO] Launching ase-sim-compile.sh"
        # NOTE: this script is the same as for n6001
        # NOTE: this script strictly relies on oneapi-asp's tag: ofs-2023.3-2
        source ${OFS_ASP_ROOT}/hardware/ofs_nc220/build/scripts/ase-sim-compile.sh
        # Remove spurious output
        rm -rf ../../../../../../fpga.bin simulation.tar.gz

        cd ${SYCL_AFU_DIR}

        # Generate new file for source list
        SIM_AFU_SOURCE_LIST=tmp_$(basename ${AFU_SOURCE_LIST} .txt )_sim.txt
        cp ${AFU_SOURCE_LIST} ${SIM_AFU_SOURCE_LIST}
        # Append header line
        printf "\n########################################################" >> ${SIM_AFU_SOURCE_LIST}
        printf "\n# Force Questa to include files from kernel_system.qip #" >> ${SIM_AFU_SOURCE_LIST}
        printf "\n########################################################" >> ${SIM_AFU_SOURCE_LIST}
        printf "\n\n# Generated from sim_files tree\n" >> ${SIM_AFU_SOURCE_LIST}

        # Append new list, inject compile order
        SIM_FILES_DIR=${SYCL_IP_NAME}_report.prj/sim_files

        # NOTE: Injecting the compile order is a dirty workaround, but this is just a PoC
        # TODO: there is some redundancy here, remove replicates
        echo "# Fist acl_* primitives" >> ${SIM_AFU_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "acl_ecc_pkg.sv" | sort >> ${SIM_AFU_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "acl_*" | grep -v acl_ecc_pkg | sort >> ${SIM_AFU_SOURCE_LIST}
        
        echo "# Fist hld_* primitives" >> ${SIM_AFU_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "hld_*" | sort >> ${SIM_AFU_SOURCE_LIST}

        echo "# All others, exclude acl primitives and IP" >> ${SIM_AFU_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f               | grep -Ev "_report_di|RSErasureID|acl|hld|inst|kernel_system\.v" | sort >> ${SIM_AFU_SOURCE_LIST}

        echo "# SYCL IP internal"
        find ${SIM_FILES_DIR} -type f -name "*RSErasureID*"  | sort >> ${SIM_AFU_SOURCE_LIST}

        # echo "# Finally, SYCL IP and wrapper"
        # find ${SIM_FILES_DIR} -type f -name "*${SYCL_IP_NAME}*" | grep -Ev "inst|RSErasureID|sys" | sort >> ${SIM_AFU_SOURCE_LIST}
        echo "# Finally, SYCL IP wrapper"
        find ${SIM_FILES_DIR} -type f -name "kernel_system.v" >> ${SIM_AFU_SOURCE_LIST}

        # Override old filename
        AFU_SOURCE_LIST=$(realpath ${SIM_AFU_SOURCE_LIST})
    fi
fi

cd ${AFU_FLOW_DIR}

# Setup optional flags
# Verbosity
if [ "$VERBOSE" = "1" ]; then
    OPT_FLAGS="${OPT_FLAGS} --ase-verbose"
fi
# Simuation mode
if [ "$ASE_MODE" != "" ]; then
    OPT_FLAGS="${OPT_FLAGS} --ase-mode ${ASE_MODE}"
fi

# Launch script
afu_sim_setup                       \
    --sources ${AFU_SOURCE_LIST}    \
    --tool QUESTA                   \
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
