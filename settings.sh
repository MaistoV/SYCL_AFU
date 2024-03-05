# Initial setup
# Root directory of current project, same path as this script
export ROOT_DIR=$( dirname $( realpath ${BASH_SOURCE[0]} ) )
# Directory for installation builds
export INSTALL_BUILD_DIR=${INSTALL_BUILD_DIR=${ROOT_DIR}/install/build}
# Directory for Hitek release, parent of ofs-agx7-pcie-attach
export HTS_RELEASE=${HTS_RELEASE=${ROOT_DIR}/hitek_release/AG_C220_NC220_OFS_Release_v1_0_2024-01-22/htk_ofs_nc220}
# PCIe address of PAC (if not set)
export PAC_PCIE_SBD=${PAC_PCIE_SBD:="0000:8a:00"}
# Remove segment value
export PAC_PCIE_BD=$(echo $PAC_PCIE_SBD | awk -F ':' '{print $2 ":" $3}')

# For tests 
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

export FIM_NUM_PF0_VFS=10
export OFSS_CONFIG=pf0_${FIM_NUM_PF0_VFS}vf
# export FIM_STATIC_AFUS=4 # VFs 1..4
# export FIM_TOT_AFUS=$((${FIM_NUM_PF0_VFS} + ${FIM_STATIC_AFUS}))

#####################
# FIM/AFU synthesis #
#####################
export FIM_BUILD_DIR=$HTS_RELEASE/ofs-agx7-pcie-attach/work_htk_nc220_${FPGA}_$OFSS_CONFIG/

export OFS_ROOTDIR=$HTS_RELEASE/ofs-agx7-pcie-attach

# If not already done, export OFS_BUILD_ROOT to the top level directory for AFU development
export OFS_BUILD_ROOT=$HTS_RELEASE/ofs-agx7-pcie-attach

# If not already done, export OPAE_PLATFORM_ROOT to the PR build tree directory
# export OPAE_PLATFORM_ROOT=$HTS_RELEASE/ofs-agx7-pcie-attach/work_htk_nc220_${FPGA}/pr_build_template
# export OPAE_PLATFORM_ROOT=$HTS_RELEASE/ofs-agx7-pcie-attach/work_htk_nc220_${FPGA}_$OFSS_CONFIG/pr_build_template
export OPAE_PLATFORM_ROOT=$HTS_RELEASE/prebuild_images/agf014/release_v1.1/pr_build_template

# OPAE SDK release
export OPAE_SDK_VERSION=2.8.0-1
export OPAE_SDK_REPO_BRANCH=release/$OPAE_SDK_VERSION

# The following environment variables are required for compiling the AFU examples. 

# Location to clone the ofs-platform-afu-bbb repository which contains PIM files and AFU examples.
export OFS_PLATFORM_AFU_BBB=$OFS_BUILD_ROOT/external/ofs-platform-afu-bbb 

# Location to the example-afu clone
export EXAMPLES_AFU=$OFS_BUILD_ROOT/external/examples-afu

# OPAE and MPF libraries must either be on the default linker search paths or on both LIBRARY_PATH and LD_LIBRARY_PATH.  
export OPAE_LOC=/usr
export LIBRARY_PATH=$OPAE_LOC/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$OPAE_LOC/lib64:$LD_LIBRARY_PATH

export FIM_IMAGE_USER1=${OPAE_PLATFORM_ROOT}/hw/blue_bits/ofs_top_page1_unsigned_user1.bin
export FIM_IMAGE_USER2=${OPAE_PLATFORM_ROOT}/hw/blue_bits/ofs_top_page2_unsigned_user2.bin

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

# For QuestaSIM, set the following:
export MTI_HOME=/home/mentor/questa_core_2020/questasim/
export PATH=$MTI_HOME/linux_x86_64/:$MTI_HOME/bin/:$PATH

############
# AFU flow # 
############
export AFU_NAME=my_custom_afu_array
source ${ROOT_DIR}/afu_flow/settings_afu.sh

##########
# OneAPI #
##########

export ONEAPI_ROOT=/opt/intel/oneapi

export QUARTUS_ROOTDIR_OVERRIDE=$QUARTUS_ROOTDIR
# Other OFS environment variables
export WORKDIR=$OFS_ROOTDIR
export LIBOPAE_C_ROOT=/usr 

source ${ONEAPI_ROOT}/setvars.sh

# OneAPI ASP
export OFS_ASP_ROOT="$HTS_RELEASE/oneapi/oneapi-asp_agf014/nc220"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OFS_ASP_ROOT/linux64/lib

# From $OFS_ASP_ROOT/hardware/
# export OFS_ASP_BOARD_VARIANT=ofs_nc220
# export OFS_ASP_BOARD_VARIANT=ofs_nc220_iopipes
export OFS_ASP_BOARD_VARIANT=ofs_nc220_usm
# export OFS_ASP_BOARD_VARIANT=ofs_nc220_usm_iopip

# ASP BSP OneAPI compilation flag
export OFS_ASP_FPGA_DEVICE=$OFS_ASP_ROOT:$OFS_ASP_BOARD_VARIANT

############################
# OneAPI FPGA IP Authoring #  
############################

export ONEAPI_IP_DIR=${ROOT_DIR}/oneapi/rs_sycl_ip
export ONEAPI_SAMPLES_DIR=${ROOT_DIR}/oneapi/oneAPI-samples
export ONEAPI_SAMPLES_INCLUDE=${ONEAPI_SAMPLES_DIR}/DirectProgramming/C++SYCL_FPGA/include

# Non-BSP OneAPI compilation flag
export AGILEX7_PART_NUMBER=AGFB014R24C2E2V # C220 part number

########################
# Print out enviroment #  
########################
echo "Dump environment:"
# Printing all (Quartus, OpenCL SDK, GCC) versions for user info
echo ""
quartus_sh -v
echo ""
icpx --version # (for Intel® oneAPI Base Toolkit (Base Kit))
echo ""
gcc --version | grep gcc --color=none
echo ""

echo MTI_HOME           : $MTI_HOME
echo OPAE_PLATFORM_ROOT : $(basename $(dirname $OPAE_PLATFORM_ROOT))
echo AFU_NAME           : $AFU_NAME
echo OFSS_CONFIG        : $OFSS_CONFIG
echo PAC_PCIE_SBD       : $PAC_PCIE_SBD