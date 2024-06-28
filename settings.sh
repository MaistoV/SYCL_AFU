#!/bin/bash

#################
# Initial setup #
#################
# Root directory of current project, same path as this script
export ROOT_DIR=$( dirname $( realpath ${BASH_SOURCE[0]} ) )
# Directory for tools installation builds
export INSTALL_BUILD_DIR=${INSTALL_BUILD_DIR=${ROOT_DIR}/install/build}
# Directory for Hitek release, parent of ofs-agx7-pcie-attach
export HTS_RELEASE=${HTS_RELEASE=${ROOT_DIR}/hitek_release/AG_C220_NC220_OFS_Release_v1_0_2024-01-22/htk_ofs_nc220}
# PCIe address of PAC (if not set)
export PAC_PCIE_SBD=${PAC_PCIE_SBD:="0000:8a:00"}
# PCIe bus:device address, remove segment value
export PAC_PCIE_BD=$(echo $PAC_PCIE_SBD | awk -F ':' '{print $2 ":" $3}')
# Hitek Board
export BOARD=htk-nc220-agf014

#############
# HEM tests #
#############
export HEM_OUT_DIR=$(pwd)/HEM/results 

#################
# Quartus Tools # 
#################
# Note, QUARTUS_HOME is your Quartus installation directory, e.g. $QUARTUS_HOME/bin contains Quartus executable.
: ${QUARTUS_HOME=~/intelFPGA_pro/23.2/quartus}
export QUARTUS_ROOTDIR=$QUARTUS_HOME
export QUARTUS_INSTALL_DIR=$QUARTUS_ROOTDIR
export QUARTUS_ROOTDIR_OVERRIDE=$QUARTUS_ROOTDIR
export IMPORT_IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
export IP_ROOTDIR=$QUARTUS_ROOTDIR/../ip
export QSYS_ROOTDIR=$QUARTUS_ROOTDIR/../qsys
# TODO FIX ME, can't find quartus from here
export PATH=$QUARTUS_HOME/bin:$QSYS_ROOTDIR/bin:$QUARTUS_HOME/../sopc_builder/bin/:$PATH

# Hitek boards
# export BOARD_VAR=agf008
export BOARD_VAR=agf014
# export BOARD_VAR=agf027
export FPGA="$BOARD_VAR"

#################
# OFSS FIM flow # 
#################
export NO_HEMS=${NO_HEMS=1}

export FIM_NUM_PF0_VFS=${FIM_NUM_PF0_VFS=4}
export OFSS_CONFIG=pf0_${FIM_NUM_PF0_VFS}vf
# First VF number exposing AFU logic
export FIRST_AFU_VF=5
# If HEMs are removed
if [[ $NO_HEMS == 1 ]]; then
    export OFSS_CONFIG=${OFSS_CONFIG}_no_hems
    export FIRST_AFU_VF=1
fi

# For simplicity, always build from custom OFSS configs
export OFSS_CONFIG_DIR=${ROOT_DIR}/fim_flow/ofss_configs/ofss_config_${OFSS_CONFIG}

# Adjust max number of AFUs
export AFU_MAX_NUM=${AFU_MAX_NUM=$FIM_NUM_PF0_VFS}

#####################
# FIM/AFU synthesis #
#####################
# import script fim_flow/resize_pr/resize_pr_assignments.tcl
export RESIZE_PR=${RESIZE_PR=1}

# Import custom AFU as default
# Both for flat and PR FIM builds
export UPDATE_DEFAULT_AFU=${UPDATE_DEFAULT_AFU=0}

unset RESEED_FITTER

# Some synonyms for different flows here
export OFS_ROOTDIR=${HTS_RELEASE}/ofs-agx7-pcie-attach
# OFS_BUILD_ROOT to the top level directory for AFU development
export OFS_BUILD_ROOT=${OFS_ROOTDIR}
export BUILD_ROOT_REL=${OFS_ROOTDIR}

# Target build directory for FIM
FIM_BASE_BUILD_DIR=${OFS_ROOTDIR}/work_htk_nc220_${FPGA}_${OFSS_CONFIG}
# For flat builds
export FIM_FLAT_BUILD_DIR=${FIM_BASE_BUILD_DIR}_flat
# For PR builds
export FIM_PR_BUILD_DIR=${FIM_BASE_BUILD_DIR}
if [[ $RESIZE_PR == 1 ]]; then
    export FIM_PR_BUILD_DIR=${FIM_PR_BUILD_DIR}_RESIZE_PR
fi

# OPAE_PLATFORM_ROOT to the PR build tree directory
# export OPAE_PLATFORM_ROOT=${OPAE_PLATFORM_ROOT=${OFS_ROOTDIR}/work_htk_nc220_${FPGA}/pr_build_template}
# export OPAE_PLATFORM_ROOT=${OPAE_PLATFORM_ROOT=${OFS_ROOTDIR}/work_htk_nc220_${FPGA}_${OFSS_CONFIG}/pr_build_template}
# export OPAE_PLATFORM_ROOT=${OPAE_PLATFORM_ROOT=${HTS_RELEASE}/prebuild_images/agf014/release_v1.1/pr_build_template}
export OPAE_PLATFORM_ROOT=${OPAE_PLATFORM_ROOT=${FIM_PR_BUILD_DIR}/pr_build_template}

# FIM image ID
export FIM_IMAGE_INFO=$(cat ${OPAE_PLATFORM_ROOT}/hw/lib/build/syn/board/htk-nc220-agf014/syn_top/user1_image_info.txt)
# PR-tree ID
export PR_INTERFACE_ID=$(cat ${OPAE_PLATFORM_ROOT}/hw/lib/fme-ifc-id.txt)

# FIM image names
export FIM_IMAGE_USER1=${OPAE_PLATFORM_ROOT}/hw/blue_bits/ofs_top_page1_unsigned_user1.bin
export FIM_IMAGE_USER2=${OPAE_PLATFORM_ROOT}/hw/blue_bits/ofs_top_page2_unsigned_user2.bin

# OPAE SDK release
export OPAE_SDK_VERSION=2.8.0-1
export OPAE_SDK_REPO_BRANCH=release/$OPAE_SDK_VERSION

# The following environment variables are required for compiling the AFU examples. 

# Location to clone the ofs-platform-afu-bbb repository which contains PIM files and AFU examples.
export OFS_PLATFORM_AFU_BBB=$OFS_BUILD_ROOT/external/ofs-platform-afu-bbb 

# Location to the example-afu clone
export EXAMPLES_AFU=${ROOT_DIR}/afu_flow/afus/examples-afu

# OPAE and MPF libraries must either be on the default linker search paths or on both LIBRARY_PATH and LD_LIBRARY_PATH.  
export OPAE_LOC=/usr
export LIBRARY_PATH=$OPAE_LOC/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$OPAE_LOC/lib64:$LD_LIBRARY_PATH
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OPAE_LOC/lib64
# export LIBRARY_PATH=$LIBRARY_PATH:$OPAE_LOC/lib

############
# ASE only #
############
# Add -debugdb to vsim builds
# export DEBUG_VSIM=1 # Currently not working  

# Setup continuous mode
export ASE_MODE=3

# Is this necessary?
export PATH=/usr/bin:$PATH
cd /usr/lib/python*/site-packages
export PYTHONPATH=$PWD
cd $ROOT_DIR
export LIBRARY_PATH=/usr/lib
export LD_LIBRARY_PATH=/usr/lib64

# For Mentor QuestaSim, set the following:
# export MTI_HOME=/home/mentor/questa_core_2020/questasim/
# For Intel Questa, set the following:
export MTI_HOME=/home/vincenzo/intelFPGA_pro/23.2/questa_fe
export PATH=$MTI_HOME/linux_x86_64/:$MTI_HOME/bin/:$PATH

############
# AFU flow # 
############
# Utility AFUs
# export AFU_NAME=${AFU_NAME="my_custom_afu"}
# export AFU_NAME=${AFU_NAME="mixed_intf_afu_array"}

# Target AFU
export AFU_NAME=${AFU_NAME="sycl_rs_erasure"}
# export AFU_NAME=${AFU_NAME="sycl_rs_erasure_array"}
# Don't export RS_SCHEMA for AFUs other than sycl_rs_erasure
if [ "${AFU_NAME}" == "sycl_rs_erasure" ] ||
    [ "${AFU_NAME}" == "sycl_rs_erasure_array" ]; then
    export RS_SCHEMA=${RS_SCHEMA=RS_3_2}

    # Override AFU_MAX_NUM
    # case ${RS_SCHEMA} in
    #     "RS_3_2")
    #         export AFU_MAX_NUM=13
    #         ;;
    #     "RS_6_3")
    #         export AFU_MAX_NUM=5
    #         ;;
    #     "RS_10_4")
    #         export AFU_MAX_NUM=1?
    #         ;;
    # esac
else 
    unset RS_SCHEMA
fi

# AFU-specific settings
# Sets AFU_PARAMS
source ${ROOT_DIR}/afu_flow/settings_afu.sh

# Import custom AFU flow into FIM
unset AFU_WITH_PIM
if [[ $UPDATE_DEFAULT_AFU == 1 ]]; then
    # Hook AFU into FIM
    export AFU_WITH_PIM=${AFU_SOURCE_LIST}
    # Append AFU_PARAMS
    export FIM_FLAT_BUILD_DIR=${FIM_FLAT_BUILD_DIR}_${AFU_PARAMS}
    export FIM_PR_BUILD_DIR=${FIM_PR_BUILD_DIR}_${AFU_PARAMS}
fi

# Utility IDs and paths
export AFU_GBS_FILE=${AFU_SYNTH_DIR}/${AFU_NAME}.gbs
export AFU_ID=$(grep uuid ${AFU_HW_DIR}/${AFU_NAME}.json | awk '{print $2}' | sed "s/\"//g")
export AFU_FIM_IMAGE_INFO=$(cat ${AFU_SYNTH_DIR}/build/quartus_proj_dir/user1_image_info.txt )
export AFU_PR_INTERFACE_ID=$(grep "FME_IFC_ID=" ${AFU_SYNTH_DIR}/build/quartus_proj_dir/build_env_db.txt | sed "s/FME_IFC_ID=//g")
# Collect in a single variable
export AFU_ENV="
    - AFU_NAME            = ${AFU_NAME}
    - AFU_PARAMS          = ${AFU_PARAMS}
    - AFU_ID              = ${AFU_ID}
    - AFU_FIM_IMAGE_INFO  = ${AFU_FIM_IMAGE_INFO}
    - AFU_PR_INTERFACE_ID = ${AFU_PR_INTERFACE_ID}
"

###############
# OneAPI Base #
###############
export ONEAPI_ROOT=${ONEAPI_ROOT=/opt/intel/oneapi}
export QUARTUS_ROOTDIR_OVERRIDE=$QUARTUS_ROOTDIR
# Other OFS environment variables
export WORKDIR=$OFS_ROOTDIR
export LIBOPAE_C_ROOT=/usr 

# Setup OneAPI Base Toolkit, force re-execution
source ${ONEAPI_ROOT}/setvars.sh --force

# SYCL IP names and working directories
export SYCL_IP_NAME=${AFU_NAME} # Use same name as AFU
# Directory of sources
export SYCL_SRC_DIR=${ROOT_DIR}/oneapi/${SYCL_IP_NAME}
export SYCL_IP_BUILD_DIR=${SYCL_SRC_DIR}/build_ip
export SYCL_ASP_BUILD_DIR=${SYCL_SRC_DIR}/build_asp
# Append RS_SCHEMA (if set)
if [[ "${RS_SCHEMA}" != "" ]]; then
    export SYCL_IP_NAME=${SYCL_IP_NAME}_${RS_SCHEMA}
    export SYCL_IP_BUILD_DIR=${SYCL_IP_BUILD_DIR}_${RS_SCHEMA}
    export SYCL_ASP_BUILD_DIR=${SYCL_ASP_BUILD_DIR}_${RS_SCHEMA}
fi

############################
# OneAPI FPGA IP Authoring #  
############################

# OneAPI includes
export ONEAPI_SAMPLES_DIR=${ROOT_DIR}/oneapi/oneAPI-samples
export ONEAPI_SAMPLES_INCLUDE=${ONEAPI_SAMPLES_DIR}/DirectProgramming/C++SYCL_FPGA/include

# Original project path
export SYCL_IP_PRJ=${SYCL_IP_BUILD_DIR}/${SYCL_IP_NAME}_report.prj
export SYCL_IP_PRJ_ARCHIVE=${SYCL_IP_BUILD_DIR}/${SYCL_IP_NAME}_report.a
# Exported project path
export SYCL_IP_PRJ_AFU_EXPORT=${AFU_HW_DIR}/${SYCL_IP_NAME}_report.prj

# Non-BSP (non-ASP) OneAPI compilation flag
export AGILEX7_PART_NUMBER=AGFB014R24C2E2V # C220 part number
# Offset of the SYCL kernel CSR space
export KERNEL_REGISTER_MAP_OFFSET_HEX=100
# Disable AVMM interrupt injection
export DISABLE_AVMM_INTERRUPT=0

##############
# OneAPI ASP #
##############
export OFS_ASP_ROOT=${HTS_RELEASE}/oneapi/oneapi-asp_agf014/nc220

# ASP variant, from tree $OFS_ASP_ROOT/hardware/
# export OFS_ASP_BOARD_VARIANT=${OFS_ASP_BOARD_VARIANT=ofs_nc220}
# export OFS_ASP_BOARD_VARIANT=${OFS_ASP_BOARD_VARIANT=ofs_nc220_iopipes}
export OFS_ASP_BOARD_VARIANT=${OFS_ASP_BOARD_VARIANT=ofs_nc220_usm}
# export OFS_ASP_BOARD_VARIANT=${OFS_ASP_BOARD_VARIANT=ofs_nc220_usm_iopipes}

# Root of built aocx
export AOCX_ROOT=${OFS_ASP_ROOT}/build/bringup/${OFS_ASP_BOARD_VARIANT}

# Set USM flag
unset OFS_ASP_USM_FLAG
if [[ "${OFS_ASP_BOARD_VARIANT}" == *"usm"* ]]; then
    export OFS_ASP_USM_FLAG="-DIS_USM=1"
fi

# Utility IDs
export AOCX_PR_INTERFACE_ID=$(cat ${OFS_ASP_ROOT}/pr_build_template/hw/lib/fme-ifc-id.txt)
export AOCX_FIM_IMAGE_INFO=$(cat ${OFS_ASP_ROOT}/pr_build_template/hw/lib/build/syn/board/htk-nc220-agf014/syn_top/user1_image_info.txt)

# ASP BSP OneAPI compilation flag
export OFS_ASP_FPGA_DEVICE=$OFS_ASP_ROOT:$OFS_ASP_BOARD_VARIANT
# Append board variant
export SYCL_ASP_BUILD_DIR=${SYCL_ASP_BUILD_DIR}_${OFS_ASP_BOARD_VARIANT}
# Append ASP_ZERO_COPY
export ASP_ZERO_COPY=${ASP_ZERO_COPY=1}
if [[ $ASP_ZERO_COPY == 1 ]]; then
    export SYCL_ASP_BUILD_DIR=${SYCL_ASP_BUILD_DIR}_ASP_ZERO_COPY
fi 
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OFS_ASP_ROOT/linux64/lib

############
# Measures #
############
# Select the exeriments profile
# export EXPERIMENT_PROFILE=${EXPERIMENT_PROFILE="1MB"} # Only one cell length
export EXPERIMENT_PROFILE=${EXPERIMENT_PROFILE="QUICK"} # Reduced number of samples
# export EXPERIMENT_PROFILE=${EXPERIMENT_PROFILE="COMPLETE"}  # Full set of samples

# Number of repetitions
export MEASURE_NUM_REPS=${MEASURE_NUM_REPS=3} 
# Number of reconstructions per run
export MEASURE_MAX_DECODE=${MEASURE_MAX_DECODE=10} 

####################
# Measures Latency #
####################
export MEASURE_LATENCY_DIR=${ROOT_DIR}/measures/latency
export MEASURE_LATENCY_DATA_DIR=${MEASURE_LATENCY_DIR}/data
export MEASURE_LATENCY_PLOT_OUT_DIR=${MEASURE_LATENCY_DIR}/plots/output_plots

###################
# Measures Cycles #
###################
export MEASURE_CYCLES_DATA_DIR=${MEASURE_LATENCY_DIR}/data/data_SYCL_ASP_SIM_ASP_ZERO_COPY
export MEASURE_CYCLES_PLOT_OUT_DIR=${MEASURE_LATENCY_DIR}/plots/output_plots

##################
# Measures Power #
##################
export MEASURE_POWER_DATA_DIR=${ROOT_DIR}/measures/power
export MEASURE_POWER_PLOT_OUT_DIR=${MEASURE_POWER_DATA_DIR}/plots/output_plots

#################
# Multi-erasure #
#################

# Use multi-erasure source code
export MULTI_ERASURE_SIMPLE=${MULTI_ERASURE_SIMPLE=1}
# Append MULTI_ERASURE_SIMPLE_SUFFIX (if MULTI_ERASURE_SIMPLE is set)
if [[ ${MULTI_ERASURE_SIMPLE} == 1 ]]; then
    MULTI_ERASURE_SIMPLE_SUFFIX="_multi_erasure"
    export MEASURE_LATENCY_DATA_DIR=${MEASURE_LATENCY_DATA_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
    export MEASURE_LATENCY_PLOT_OUT_DIR=${MEASURE_LATENCY_PLOT_OUT_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
    export MEASURE_CYCLES_DATA_DIR=${MEASURE_CYCLES_DATA_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
    export MEASURE_CYCLES_PLOT_OUT_DIR=${MEASURE_CYCLES_PLOT_OUT_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
    export MEASURE_POWER_DATA_DIR=${MEASURE_POWER_DATA_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
    export MEASURE_POWER_PLOT_OUT_DIR=${MEASURE_POWER_PLOT_OUT_DIR}${MULTI_ERASURE_SIMPLE_SUFFIX}
fi

########################
# Print out enviroment #  
########################
# echo "Dump environment:"
# # Printing all (Quartus, OpenCL SDK, GCC) versions for user info
# echo ""
# quartus_sh -v
# echo ""
# icpx --version # (for Intel® oneAPI Base Toolkit (Base Kit))
# echo ""
# gcc --version | grep gcc --color=none
# echo ""

echo "PAC_PCIE_SBD          : $PAC_PCIE_SBD"
echo "OFSS_CONFIG           : $OFSS_CONFIG"
echo "OPAE_PLATFORM_ROOT    : $(basename $(dirname $OPAE_PLATFORM_ROOT))"
echo "OFS_ASP_BOARD_VARIANT : $OFS_ASP_BOARD_VARIANT"
echo "AOCX_PR_INTERFACE_ID  : $AOCX_PR_INTERFACE_ID"
echo "AOCX_FIM_IMAGE_INFO   : $AOCX_FIM_IMAGE_INFO"
echo "RS_SCHEMA             : $RS_SCHEMA"
echo "MUTLI_ERASURE_SIMPLE  : $MULTI_ERASURE_SIMPLE"
echo "EXPERIMENT_PROFILE    : $EXPERIMENT_PROFILE"
echo "AFU_ENV                 $AFU_ENV"
