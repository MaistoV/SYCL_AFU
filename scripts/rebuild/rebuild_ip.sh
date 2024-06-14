# Rebuild IP
# RS_3_2
export AFU_NAME="sycl_rs_erasure" 
export RS_SCHEMA=RS_3_2
source settings.sh > /dev/null
make clean_oneapi_ip       
make oneapi_ip

# RS_6_3
export RS_SCHEMA=RS_6_3
source settings.sh > /dev/null
make clean_oneapi_ip       
make oneapi_ip

# # RS_10_4
# export RS_SCHEMA=RS_10_4
# source settings.sh &> /dev/null
# make clean_oneapi_ip       
# make oneapi_ip
