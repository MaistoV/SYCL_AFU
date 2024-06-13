
# 1. ASP (on pre-built)
export OPAE_PLATFORM_ROOT=${HTS_RELEASE}/prebuild_images/agf014/release_v1.1/pr_build_template

#   1.1 ASP RS_3_2 ASP_ZERO_COPY
export RS_SCHEMA=RS_3_2
export ASP_ZERO_COPY=1
source settings.sh > /dev/null
make clean_oneapi_asp
make oneapi_asp_report oneapi_asp_fpga &> logs/oneapi_asp_fpga.ASP_ZERO_COPY.${AFU_PARAMS}.log

#   1.2 ASP RS_6_3 ASP_ZERO_COPY
export ASP_ZERO_COPY=1
export RS_SCHEMA=RS_6_3
source settings.sh > /dev/null
make clean_oneapi_asp
make oneapi_asp_report oneapi_asp_fpga &> logs/oneapi_asp_fpga.ASP_ZERO_COPY.${AFU_PARAMS}.log

# #   1.2 ASP RS_10_4 ASP_ZERO_COPY
# export ASP_ZERO_COPY=1
# export RS_SCHEMA=RS_10_4
# source settings.sh > /dev/null
# make clean_oneapi_asp
# make oneapi_asp_report oneapi_asp_fpga &> logs/oneapi_asp_fpga.ASP_ZERO_COPY.${AFU_PARAMS}.log
