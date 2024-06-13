# Rebuild Flat VF array
export AFU_NAME="sycl_rs_erasure_array" 

#   3.3. Flat   AFU array     13 x RS_3_2
export RS_SCHEMA=RS_3_2
export UPDATE_DEFAULT_AFU=1
export FIM_NUM_PF0_VFS=14
export AFU_MAX_NUM=13
source settings.sh > /dev/null
make clean_fim_flat
make fim_build_flat > fim_build_flat.${AFU_PARAMS}.log

#   3.4. Flat   AFU array      6 x RS_6_3
export RS_SCHEMA=RS_6_3
export UPDATE_DEFAULT_AFU=1
export FIM_NUM_PF0_VFS=6
export AFU_MAX_NUM=6
source settings.sh > /dev/null
make clean_fim_flat
make fim_build_flat > fim_build_flat.${AFU_PARAMS}.log

# # RS_10_4
# TBD