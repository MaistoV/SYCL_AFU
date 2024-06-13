# Rebuild GBSes
# Need to set OPAE_PLATFORM_ROOT
source settings.sh > /dev/null
export OPAE_PLATFORM_ROOT=${HTS_RELEASE}/prebuild_images/agf014/release_v1.1/pr_build_template

# #   2.1. GBS RS_3_2 (on pre-built)
export RS_SCHEMA=RS_3_2
source settings.sh > /dev/null
make clean_gbs
make gbs &> gbs.${AFU_PARAMS}.log

#   2.2. GBS RS_6_3 (on pre-built)
export RS_SCHEMA=RS_6_3
source settings.sh > /dev/null
make clean_gbs
make gbs &> gbs.${AFU_PARAMS}.log

# # RS_10_4
# export RS_SCHEMA=RS_10_4
# source settings.sh > /dev/null
# make clean_gbs
# make gbs &> gbs.${AFU_PARAMS}.log