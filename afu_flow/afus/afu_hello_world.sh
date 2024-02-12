# AFU interface type in = { 01_pim_ifc, 02_hybrid, 03_afu_main }
AFU_TYPE="01_pim_ifc"
# AFU interface type in = { avalon, axi, ccip }
AFU_INTF=axi

# AFU name
export AFU_NAME=hello_world_${AFU_TYPE}_${AFU_INTF}
export AFU_ELF_NAME=hello_world
# Source list file
export AFU_SOURCE_LIST=$EXAMPLES_AFU/tutorial/afu_types/${AFU_TYPE}/hello_world/hw/rtl/${AFU_INTF}/sources.txt
# AFU-related software directory
export AFU_SW_DIR=$EXAMPLES_AFU/tutorial/afu_types/${AFU_TYPE}/hello_world/sw