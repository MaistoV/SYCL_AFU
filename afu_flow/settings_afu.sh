################################
# AFU-common working directory # 
################################
export AFU_FLOW_DIR=${ROOT_DIR}/afu_flow
export AFU_DEF_DIR=${AFU_FLOW_DIR}/afus
export AFU_BUILD_DIR=${AFU_FLOW_DIR}/build
mkdir -p ${AFU_BUILD_DIR}

##################
# AFU aspecifics #
##################
# RTL source list file  
export AFU_SOURCE_LIST=${AFU_DEF_DIR}/${AFU_NAME}/hw/rtl/sources.txt
# AFU-related hardware directory
export AFU_HW_DIR=${AFU_DEF_DIR}/${AFU_NAME}/hw/rtl
# AFU-related software directory (location of sofware sources and Makefile)
export AFU_SW_DIR=${AFU_DEF_DIR}/${AFU_NAME}/sw

####################################
# AFU-specific working directories # 
####################################
if [ "${RS_SCHEMA}" != "" ]; then
    export AFU_SYNTH_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_${RS_SCHEMA}_synth
    export AFU_ASE_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_${RS_SCHEMA}_ase
else
    export AFU_SYNTH_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_synth
    export AFU_ASE_DIR=${AFU_BUILD_DIR}/${AFU_NAME}_ase
fi
export ASE_WORKDIR=${AFU_ASE_DIR}/work

##############
# IP imports #
##############
export SYCL_AFU_COMMON=${AFU_FLOW_DIR}/afus/sycl_afu_common
export AVMM_SPLITTER_PRJ=${SYCL_AFU_COMMON}/hw/qsys/avmm_splitter
