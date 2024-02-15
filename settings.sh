# Initial setup
export ROOT_DIR=$(pwd)
export OUT_DIR=$(pwd)/results
export UTILS_BUILD_DIR=$(pwd)/install/build

# : ${HTS_FIM_RELEASE=$(pwd)/hitek_release/AG_C220_NC220_OFS_Release_v1_0_2024-01-22/htk_ofs_nc220}
export HTS_FIM_RELEASE=$(pwd)/hitek_release/AG_C220_NC220_OFS_Release_v1_0_2024-01-22/htk_ofs_nc220
export OFS_ROOTDIR=$HTS_FIM_RELEASE/ofs-agx7-pcie-attach


# Quartus Tools
# Note, QUARTUS_HOME is your Quartus installation directory, e.g. $QUARTUS_HOME/bin contains Quartus executable.
export QUARTUS_HOME=~/intelFPGA_pro/23.2/quartus
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
# For synthesis #
#################

# If not already done, export OFS_BUILD_ROOT to the top level directory for AFU development
export OFS_BUILD_ROOT=$HTS_FIM_RELEASE/ofs-agx7-pcie-attach

# If not already done, export OPAE_PLATFORM_ROOT to the PR build tree directory
export OPAE_PLATFORM_ROOT=$HTS_FIM_RELEASE/ofs-agx7-pcie-attach/work_htk_nc220_${FPGA}/pr_build_template/
# export OPAE_PLATFORM_ROOT=$HTS_FIM_RELEASE/prebuild_images/agf014/release_v1.1/pr_build_template

# OPAE SDK release
export OPAE_SDK_VERSION=2.8.0-1
export OPAE_SDK_REPO_BRANCH=$OPAE_SDK_VERSION

# The following environment variables are required for compiling the AFU examples. 

# Location to clone the ofs-platform-afu-bbb repository which contains PIM files and AFU examples.
export OFS_PLATFORM_AFU_BBB=$OFS_BUILD_ROOT/external/ofs-platform-afu-bbb 

# Location to the example-afu clone
export EXAMPLES_AFU=$OFS_BUILD_ROOT/external/examples-afu

# OPAE and MPF libraries must either be on the default linker search paths or on both LIBRARY_PATH and LD_LIBRARY_PATH.  
export OPAE_LOC=/usr
export LIBRARY_PATH=$OPAE_LOC/lib:$LIBRARY_PATH
export LD_LIBRARY_PATH=$OPAE_LOC/lib64:$LD_LIBRARY_PATH

############D####
# For ASE only # 
################
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
source ${ROOT_DIR}/afu_flow/settings_afu.sh

##########
# OneAPI # 
##########
# ONEAPI_WORK_DIR=...TBD....
