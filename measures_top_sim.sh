# Test plan
# ID	RS	    HW	        Erasures
# 1	    3:2	    fpga_sim	Single
# 2	    6:3	    fpga_sim	Single
# 3	    3:2	    fpga_sim	Multiple
# 4	    6:3	    fpga_sim	Multiple

# Configuration lists
declare -a MULTI_ERASURE_SIMPLE_list=(0 1)
declare -a RS_SCHEMA_list=(RS_3_2 RS_6_3)
declare -a HW_list=(asp_fpga_sim)

cnt=0
# Loop single-/multi-erasure
for erasure in "${MULTI_ERASURE_SIMPLE_list[@]}"; do
    export MULTI_ERASURE_SIMPLE=$erasure
    # Loop RS_SCHEMA
    for rs in "${RS_SCHEMA_list[@]}"; do
        export RS_SCHEMA=$rs
        source settings.sh
        cnt=$((cnt+1))
        echo "$cnt: MULTI_ERASURE_SIMPLE=$erasure, RS_SCHEMA=$rs"
        # Re-build for different MULTI_ERASURE_SIMPLE
        rm -rf ${SYCL_ASP_BUILD_DIR}/*fpga_sim*
        make oneapi_asp_fpga_sim
        # Run
        make measure_asp_fpga_sim
    done
done
