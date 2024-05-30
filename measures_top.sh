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
declare -a MULTI_THREADED_list=(0)
declare -a MULTI_ERASURE_SIMPLE_list=(0 1)
declare -a RS_SCHEMA_list=(RS_3_2 RS_6_3)
declare -a HW_list=(sycl_afu asp_fpga isal)

# Loop single-/multi-threaded
cnt=0
for threaded in "${MULTI_THREADED_list[@]}"; do
    export MULTI_THREADED=$threaded
    # Loop single-/multi-erasure
    for erasure in "${MULTI_ERASURE_SIMPLE_list[@]}"; do
        export MULTI_ERASURE_SIMPLE=$erasure
        # Loop RS_SCHEMA
        for rs in "${RS_SCHEMA_list[@]}"; do
            export RS_SCHEMA=$rs
            source settings.sh
            # Loop hw
            for hw in "${HW_list[@]}"; do
                cnt=$((cnt+1))
                echo "$cnt: MULTI_THREADED=$threaded, MULTI_ERASURE_SIMPLE=$erasure, RS_SCHEMA=$rs, HW=$hw"
                if [ $threaded -eq 1 ]; then
                    echo "make measure_multithreaded_${hw}"
                else
                    echo "make measure_${hw}"
                fi
            done
        done
    done
done
