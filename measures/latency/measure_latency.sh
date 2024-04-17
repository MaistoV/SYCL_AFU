#!/bin/bash
###############################################
# Experiment subjects:
#   * ISA-L vs SYCL kernel
# Experiment factors
#   * RS_SCHEMA
#   * cell_length
###############################################

# Check exit code, abort in case of errors
check_exit_code(){
    if [[ $1 -ne 0 ]]; then
        ${COLOR_RED}
        echo "EXIT_CODE=$1 for $2"
        ${COLOR_NORMAL}
        return 0
    fi
    return -1
}

COLOR_GREEN="tput setaf 2"
COLOR_RED="tput setaf 1"
COLOR_NORMAL="tput setaf 7"

# Measure ISA-L
decode_ISAL="0"
if [[ "$1" != "" ]]; then
    decode_ISAL=$1
fi
echo decode_ISAL = $decode_ISAL

# Nember of repetitions per run
num_runs=30
if [[ "$2" != "" ]]; then
    num_runs=$2
fi
echo num_runs = $num_runs

# Output directory
out_dir=${ROOT_DIR}/measures/latency/data/data_SYCL/
if [[ "$decode_ISAL" == "1" ]]; then 
    out_dir=${ROOT_DIR}/measures/latency/data/data_ISA_L/
fi
if [[ "$3" != "" ]]; then
    out_dir=$3
fi

# Clear old data
mkdir -p $out_dir

# Regenerate random experimental points
bash ${ROOT_DIR}/measures/latency/gen_experiments.sh $num_runs tmp.cell_length_experiments.txt 
# Read experimental point
readarray -t experiment_list < tmp.cell_length_experiments.txt 

# List of RS schemas
declare -a RS_SCHEMA_list=("RS_6_3" "RS_3_2")

# Run experiments
for RS_SCHEMA in "${RS_SCHEMA_list[@]}"
do
    export RS_SCHEMA=${RS_SCHEMA}
    source settings.sh
    echo "Build host application..."
    CMD="make ${MAKE_BUILD_TARGET} SYCL_IP_DEBUG=0 DEBUG=0"
    echo $CMD
    ${CMD}
    EXIT_CODE=$?; check_exit_code "$EXIT_CODE" "$CMD" && return $EXIT_CODE

    # Loop over experimental points
    exp=0
    for length in "${experiment_list[@]}"
    do
        exp=$(($exp + 1))
        # Launch the experiment
        # NOTE: Also re-seed PRNG with the -r flag
        export TEST_ARGS="-e 1 -d $decode_ISAL -m 1 -o $out_dir -r $(($len + $exp)) -l $length -x 1"
        CMD="make ${MAKE_TEST_TARGET}"
        echo "$RS_SCHEMA: Running experiment $exp/${#experiment_list[@]} length = $length"
        echo ${CMD} TEST_ARGS=\"${TEST_ARGS}\"
        ${CMD} > /dev/null
        EXIT_CODE=$?; check_exit_code "$EXIT_CODE" "$CMD" && return $EXIT_CODE
    done
done