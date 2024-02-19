################################
# AFU-common working directory # 
################################
export AFU_FLOW_DIR=${ROOT_DIR}/afu_flow
export AFU_DEF_DIR=${AFU_FLOW_DIR}/afus
export AFU_BUILD_DIR=${AFU_FLOW_DIR}/build
mkdir -p ${AFU_BUILD_DIR}

#######################################################################
# AFU aspecifics
# Must set the following variables:
# 1. AFU_SOURCE_LIST: e.g. location of sources.txt
# 2. AFU_SW_DIR     : location of sofware sources and Makefile
#######################################################################
# Source list file
export AFU_SOURCE_LIST=${AFU_DEF_DIR}/${AFU_NAME}/hw/rtl/sources.txt
# AFU-related software directory
export AFU_SW_DIR=${AFU_DEF_DIR}/${AFU_NAME}/sw

# AFU-dependent script
# source ${AFU_WORK_DIR}/afu_${AFU_NAME}.sh

####################################
# AFU-specific working directories # 
####################################
export AFU_SYNTH_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_afu_synth
export AFU_ASE_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_afu_ase
export ASE_WORKDIR=${AFU_ASE_DIR}/work
