# Rebuild GBSes VF array
export AFU_NAME="sycl_rs_erasure_array" 

#   3.1. GBS    AFU array     12 x RS_3_2
# Rebuild PR-tree
export FIM_NUM_PF0_VFS=13
export NO_HEMS=1
source settings.sh > /dev/null
make clean_fim_pr
make fim_build_pr &> fim_build_pr.${OFSS_CONFIG}.log
# Build GBS
export AFU_MAX_NUM=12
export RS_SCHEMA=RS_3_2
source settings.sh > /dev/null
make clean_gbs
make gbs &> gbs.${AFU_PARAMS}.log
unset OPAE_PLATFORM_ROOT

#   3.2. GBS    AFU array      4 x RS_6_3
# Rebuild PR-tree
export FIM_NUM_PF0_VFS=4
export NO_HEMS=1
export RESEED_FITTER=1
source settings.sh > /dev/null
make clean_fim_pr
make fim_build_pr &> fim_build_pr.${OFSS_CONFIG}.log
# Build GBS
export AFU_MAX_NUM=4
export RS_SCHEMA=RS_6_3
source settings.sh > /dev/null
make clean_gbs
make gbs &> gbs.${AFU_PARAMS}.log
unset RESEED_FITTER
unset OPAE_PLATFORM_ROOT