# Test plan
# ID    RS	    HW	    Threads     Erasures
# 13	3:2	    AFU	    Multiple	Single
# 14	6:3	    AFU	    Multiple	Single
# 15	3:2	    ASP	    Multiple	Single
# 16	6:3	    ASP	    Multiple	Single
# 17	3:2	    ISA-L	Multiple	Single
# 18	6:3	    ISA-L	Multiple	Single
# 19	3:2	    AFU	    Multiple	Multiple
# 20	6:3	    AFU	    Multiple	Multiple
# 21	3:2	    ASP	    Multiple	Multiple
# 22	6:3	    ASP	    Multiple	Multiple
# 23	3:2	    ISA-L	Multiple	Multiple
# 24	6:3	    ISA-L	Multiple	Multiple

# Configuration lists
declare -a MULTI_ERASURE_SIMPLE_list=(1)
# declare -a RS_SCHEMA_list=(RS_3_2 RS_6_3) # Missong 6_3
declare -a RS_SCHEMA_list=(RS_3_2)
declare -a HW_list=(sycl_afu asp_fpga)

# Number of threads
# Start simple
NUM_THREADS=4

# Choose AFU
export AFU_NAME=sycl_rs_erasure_array

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
            make measure_multi_threaded_${hw} NUM_THREADS=$NUM_THREADS
        done
    done
done
