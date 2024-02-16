################################
# AFU-common working directory # 
################################
export AFU_WORK_DIR=${ROOT_DIR}/afu_flow

#######################################################################
# AFU aspecifics
# Must set the following variables:
# 1. AFU_ELF_NAME   : for host executable
# 2. AFU_SOURCE_LIST: e.g. location of sources.txt
# 3. AFU_SW_DIR     : location of sofware sources and Makefile
#######################################################################

# AFU-dependent script
source ${AFU_WORK_DIR}/afus/afu_${AFU_NAME}.sh

####################################
# AFU-specific working directories # 
####################################
export AFU_SYNTH_DIR=${AFU_WORK_DIR}/${AFU_NAME}_afu_synth
export AFU_ASE_DIR=${AFU_WORK_DIR}/${AFU_NAME}_afu_ase
export ASE_WORKDIR=${AFU_ASE_DIR}/work
