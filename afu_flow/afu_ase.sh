#!/bin/bash

#############
# Setup ASE #
#############
# Removing old directory, if any
rm -rf ${AFU_ASE_DIR} 

# In case of SYCL-imported IP, prepare Questa file list from kernel_system.qip, using the original ASP flow
if [[ ${AFU_NAME} == *sycl* ]]; then
    cd ${AFU_HW_DIR}
    NEW_FILE_NAME=tmp_$(basename ${AFU_SOURCE_LIST} .txt )_sim.txt
    # Remove old file
    rm -f ${NEW_FILE_NAME}
    # Copy base content into new file
    cp ${AFU_SOURCE_LIST} ${NEW_FILE_NAME}

    # Import avmm_splitter
    # NOTE: Generate this dynamically, so that Platform Designer can choose any IP from its internal catalog
    if [[ "${AVMM_SPLITTER_PRJ}" != "" ]]; then
        # In case of use of avmm_splitter qsys project, prepare Questa file list from simluation exported tcl script
        
        # Append new list, inject compile order
        cd ${AFU_HW_DIR}
        AVMM_SPLITTER_SIM_FILES_DIR=${SYCL_IP_PRJ_AFU_EXPORT}/avmm_splitter_sim_files
        rm -rf ${AVMM_SPLITTER_SIM_FILES_DIR}
        mkdir -p ${AVMM_SPLITTER_SIM_FILES_DIR}

        # Copy files in local directory
        # cp $(find ${AVMM_SPLITTER_PRJ} -name "*.ip" ) ${AVMM_SPLITTER_SIM_FILES_DIR}
        cp $(find ${AVMM_SPLITTER_PRJ} -name "*.v"  | grep sim) ${AVMM_SPLITTER_SIM_FILES_DIR}
        cp $(find ${AVMM_SPLITTER_PRJ} -name "*.sv" | grep sim) ${AVMM_SPLITTER_SIM_FILES_DIR}

        # Generate new file for source list
        AVMM_SPLITTER_SOURCE_LIST=tmp_avmm_splitter_$(basename ${NEW_FILE_NAME} .txt )_sim.txt
        rm -f ${AVMM_SPLITTER_SOURCE_LIST}
        # Append header line
        printf "\n##########################################################" >> ${AVMM_SPLITTER_SOURCE_LIST}
        printf "\n# Force ASE to include files for avmm_splitter_qsys.qsys #" >> ${AVMM_SPLITTER_SOURCE_LIST}
        printf "\n##########################################################" >> ${AVMM_SPLITTER_SOURCE_LIST}
        printf "\n\n# Generated from avmm_splitter_sim_files tree\n" >> ${AVMM_SPLITTER_SOURCE_LIST}

        echo "# Top verilog source file" >> ${AVMM_SPLITTER_SOURCE_LIST}
        echo  ${AVMM_SPLITTER_PRJ}/avmm_splitter_qsys/sim/avmm_splitter_qsys.v >> ${AVMM_SPLITTER_SOURCE_LIST}

        # Append Verilog sources
        echo "# Sub Verilog sources" >> ${AVMM_SPLITTER_SOURCE_LIST}
        find ${AVMM_SPLITTER_SIM_FILES_DIR} -type f -name "*.v"  | sort >> ${AVMM_SPLITTER_SOURCE_LIST}
        # Append SystemVerilog sources
        echo "# Sub SystemVerilog sources" >> ${AVMM_SPLITTER_SOURCE_LIST}
        find ${AVMM_SPLITTER_SIM_FILES_DIR} -type f -name "*.sv" | sort >> ${AVMM_SPLITTER_SOURCE_LIST}

        # Append to file
        cat ${AVMM_SPLITTER_SOURCE_LIST} >> ${NEW_FILE_NAME}
    fi

    # Import kernel_system
    if [ -d ${SYCL_IP_PRJ_AFU_EXPORT} ]; then
        # Move to where the kernel_system.qip file is
        cd ${SYCL_IP_PRJ_AFU_EXPORT}
        echo "[INFO] Launching ase-sim-compile.sh"
        # NOTE: this script is the same as for n6001
        # NOTE: this script strictly relies on oneapi-asp's release tag: ofs-2023.3-2
        source ${OFS_ASP_ROOT}/hardware/ofs_nc220/build/scripts/ase-sim-compile.sh > /dev/null
        # Remove spurious output
        rm -rf ../../../../../../fpga.bin simulation.tar.gz

        cd ${AFU_HW_DIR}

        # Generate new file for source list
        KERNEL_SYSTEM_SOURCE_LIST=tmp_kernel_system_$(basename ${NEW_FILE_NAME} .txt )_sim.txt
        rm -f ${KERNEL_SYSTEM_SOURCE_LIST}
        # Append header line
        printf "\n#####################################################" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        printf "\n# Force ASE to include files from kernel_system.qip #" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        printf "\n#####################################################" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        printf "\n\n# Generated from sim_files tree\n" >> ${KERNEL_SYSTEM_SOURCE_LIST}

        # Append new list, inject compile order
        SIM_FILES_DIR=${SYCL_IP_PRJ_AFU_EXPORT}/sim_files

        # Add simlation-only defines
        # NOTE: AVMM interrupts trigger an error in ASE for zero-width data write
        echo "# Simlation-only defines"
        echo "+define+DISABLE_AVMM_INTERRUPT=1" >> ${KERNEL_SYSTEM_SOURCE_LIST}

        # NOTE: Injecting the compile order is a dirty workaround, but this is just a PoC
        # TODO: there is some redundancy here, remove replicates

        # acl_avalon_mm_bridge_s10
        echo "# acl_avalon_mm_bridge_s10 verilog source" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        # echo ${INTELFPGAOCLSDKROOT}/ip/board/acl_avalon_mm_bridge_s10/acl_avalon_mm_bridge_s10.v >> ${KERNEL_SYSTEM_SOURCE_LIST}
        cat ${SYCL_AFU_COMMON}/hw/filelist/sim/acl_avalon_mm_bridge_s10.sim.sources.txt >> ${KERNEL_SYSTEM_SOURCE_LIST}

        echo "# First acl_* primitives" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "acl_ecc_pkg.sv" | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "acl_*" | grep -v acl_ecc_pkg | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}
        
        echo "# First hld_* primitives" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "hld_*" | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}

        echo "# All others, exclude acl primitives and IP" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f               | grep -Ev "_report_di|ID|acl|hld|inst|kernel_system\.v" | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}

        echo "# SYCL IP internal" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "*ID*"  | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}

        echo "# SYCL IP" >> ${KERNEL_SYSTEM_SOURCE_LIST}
        find ${SIM_FILES_DIR} -type f -name "*${SYCL_IP_NAME}*" | grep -Ev "inst|ID|sys" | sort >> ${KERNEL_SYSTEM_SOURCE_LIST}
        echo "# Finally, SYCL IP wrapper"
        find ${SIM_FILES_DIR} -type f -name "kernel_system.v" >> ${KERNEL_SYSTEM_SOURCE_LIST}

        # Append to file
        cat ${KERNEL_SYSTEM_SOURCE_LIST} >> ${NEW_FILE_NAME}
    else
        echo "[ERROR] Can't find ${SYCL_IP_PRJ_AFU_EXPORT} required by SYCL AFU flow, run make oneapi_ip" >&2
        exit -1
    fi

    # Override source filename
    AFU_SOURCE_LIST=$(realpath ${NEW_FILE_NAME})
fi

# Back to afu_flow root
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
