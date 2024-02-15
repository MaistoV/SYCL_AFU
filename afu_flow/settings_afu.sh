################################
# AFU-common working directory # 
################################
export AFU_WORK_DIR=${ROOT_DIR}/afu_flow

#######################################################################
# AFU aspecifics
# Must set the following variables:
# 1. AFU_NAME       : for artifacts
# 2. AFU_ELF_NAME   : for host executable
# 3. AFU_SOURCE_LIST: e.g. location of sources.txt
# 4. AFU_SW_DIR     : location of sofware sources and Makefile
#######################################################################


# host_chan_mmio
# source ${AFU_WORK_DIR}/afus/afu_host_chan_mmio.sh

# hello_world
source ${AFU_WORK_DIR}/afus/afu_hello_world.sh

# dma
source ${AFU_WORK_DIR}/afus/dma.sh

####################################
# AFU-specific working directories # 
####################################
export AFU_SYNTH_DIR=${AFU_WORK_DIR}/${AFU_NAME}_afu_synth
export AFU_ASE_DIR=${AFU_WORK_DIR}/${AFU_NAME}_afu_ase
export ASE_WORKDIR=${AFU_ASE_DIR}/work
