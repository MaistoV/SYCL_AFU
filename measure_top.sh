# Test plan
# ID    RS	    HW	    Threads     Erasures
# 1	    3:2	    AFU	    Single	    Single
# 2	    6:3	    AFU	    Single	    Single
# 3	    3:2	    ASP	    Single	    Single
# 4	    6:3	    ASP	    Single	    Single
# 5	    3:2	    ISA-L	Single	    Single
# 6	    6:3	    ISA-L	Single	    Single
# 7	    3:2	    AFU	    Single	    Multiple
# 8	    6:3	    AFU	    Single	    Multiple
# 9	    3:2	    ASP	    Single	    Multiple
# 10	6:3	    ASP	    Single	    Multiple
# 11	3:2	    ISA-L	Single	    Multiple
# 12	6:3	    ISA-L	Single	    Multiple

# Configuration lists
declare -a MULTI_ERASURE_SIMPLE_list=(0 1)
# declare -a RS_SCHEMA_list=(RS_3_2 RS_6_3)
# declare -a HW_list=(sycl_afu asp_fpga)
declare -a RS_SCHEMA_list=(RS_3_2) # TMP
declare -a HW_list=(sycl_afu) # TMP

# Choose AFU
export AFU_NAME=sycl_rs_erasure

# Single-threaded tests rely on a HEM-equipped FIM
# For simplicity, we are using Hitek's prebuilt image
make opae.io_bind_one NO_HEMS=0

cnt=0
# Loop single-/multi-erasure
for erasure in "${MULTI_ERASURE_SIMPLE_list[@]}"; do
    export MULTI_ERASURE_SIMPLE=$erasure
    # Loop RS_SCHEMA
    for rs in "${RS_SCHEMA_list[@]}"; do
        export RS_SCHEMA=$rs
        source settings.sh > /dev/null
        # Loop hw
        for hw in "${HW_list[@]}"; do
            cnt=$((cnt+1))
            echo "$cnt: MULTI_ERASURE_SIMPLE=$erasure, RS_SCHEMA=$rs, HW=$hw"
            # Setup preconditions
            # make setup_${hw}
            # Launch measures
            make measure_${hw} SBDF=${PAC_PCIE_SBD}.${FIRST_AFU_VF}
            # Upadte plots
            make plot_latency
        done
    done
done
