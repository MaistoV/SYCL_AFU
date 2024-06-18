# Test plan
# ID    RS	    HW	    Threads     Erasures
# 13	3:2	    AFU	    Multiple	Single
# 14	6:3	    AFU	    Multiple	Single
# 15	3:2	    ISA-L	Multiple	Single
# 16	6:3	    ISA-L	Multiple	Single
# 17	3:2	    AFU	    Multiple	Multiple
# 18	6:3	    AFU	    Multiple	Multiple
# 19	3:2	    ISA-L	Multiple	Multiple
# 20	6:3	    ISA-L	Multiple	Multiple

# Configuration lists
declare -a MULTI_ERASURE_SIMPLE_list=(0 1)
declare -a RS_SCHEMA_list=(RS_3_2)
declare -a HW_list=(sycl_afu isal)

# Number of threads
# Start simple, for now must be lesser than 8
NUM_THREADS=4

# Choose AFU
# export AFU_NAME=sycl_rs_erasure_array
# NOTE: using flat FIM
export AFU_NAME=sycl_rs_erasure

# Choose experiment profile
# export EXPERIMENT_PROFILE="1MB"         # Only one cell length
export EXPERIMENT_PROFILE="QUICK"       # Reduced number of samples
# export EXPERIMENT_PROFILE="COMPLETE"    # Full set of samples

cnt=0
# Loop single-/multi-erasure
for erasure in "${MULTI_ERASURE_SIMPLE_list[@]}"; do
    export MULTI_ERASURE_SIMPLE=$erasure
    # Loop RS_SCHEMA
    for rs in "${RS_SCHEMA_list[@]}"; do
        export RS_SCHEMA=$rs
        source settings.sh &> /dev/null
        # make opae.io_release 
        # make setup_${RS_SCHEMA}_array 
        # make opae.io_bind
        make afu_host

        # Loop hw
        for hw in "${HW_list[@]}"; do
            cnt=$((cnt+1))
            echo "$cnt: MULTI_ERASURE_SIMPLE=$erasure, RS_SCHEMA=$rs, HW=$hw"
            # Setup preconditions
            # make afu_host
            # NOTE: for now running on flat FIM, so target gbs_configure would just fail
            # Launch measures
            make measure_multi_thread_${hw} NUM_THREADS=$NUM_THREADS
            # Update plots
            make plot_multi_thread
        done
    done
done
