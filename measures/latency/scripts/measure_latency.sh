#!/bin/bash
###############################################
# Experiment factors
#   * cell_length
###############################################

# Check exit code, abort in case of errors
# check_exit_code(){
#     if [[ $1 -ne 0 ]]; then
#         ${COLOR_RED}
#         echo "EXIT_CODE=$1 for $2"
#         ${COLOR_NORMAL}
#         return 0
#     fi
#     return -1
# }

# COLOR_GREEN="tput setaf 2"
# COLOR_RED="tput setaf 1"
# COLOR_NORMAL="tput setaf 7"

##############
# Parse args #
##############

# Measure ISA-L, for -d flag
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

# Output directory for -o flag
out_dir=${ROOT_DIR}/measures/latency/data/data_ASP_SYCL/
if [[ "$decode_ISAL" == "1" ]]; then 
    out_dir=${ROOT_DIR}/measures/latency/data/data_ISA_L/
fi
if [[ "$3" != "" ]]; then
    out_dir=$3
fi

# Maximum cells to decode for -c flag
if [[ "$4" != "" ]]; then
    MAX_DECODE=$4
fi

# Make targets
MAKE_TEST_TARGET=test_sycl_afu
if [[ "$5" != "" ]]; then
    MAKE_TEST_TARGET=$5
fi

# Experiment profile
EXPERIMENT_PROFILE=QUICK
if [[ "$6" != "" ]]; then
    EXPERIMENT_PROFILE=$6
fi

# AFU PCIe address
SBDF="0000:01:00.1"
if [[ "$6" != "" ]]; then
    SBDF=$7
fi

############################
# Generate experiment list #
############################

# Temporary experiments file
exp_file=tmp.cell_length_experiments.txt
# Append pid for MULTI_THREADing
if [ ${MULTI_THREAD} -eq 1 ]; then
  exp_file=${exp_file}_$$
fi

# Regenerate random experimental points
bash ${ROOT_DIR}/measures/latency/scripts/gen_experiments.sh \
    $num_runs               \
    $exp_file               \
    ${EXPERIMENT_PROFILE}

# Read experimental point
readarray -t experiment_list < $exp_file
if [ ${#experiment_list[@]} -eq 0 ]; then
    echo "[ERROR] Experiment list is empty, aborting..." >&2
    exit -1
fi

#############
# Run tests #
#############

# Create output directory
mkdir -p $out_dir

# Loop over experimental points
exp=0
for length in "${experiment_list[@]}"
do
    exp=$(($exp + 1))
    # Launch the experiment
    # NOTE: Also re-seed PRNG with the -r flag
    export TEST_ARGS="-e 1 -d $decode_ISAL -m 1 -o $out_dir -r $(($len + $exp)) -l $length -c $MAX_DECODE -f $SBDF"
    CMD="make ${MAKE_TEST_TARGET}"
    echo "[$$]: $RS_SCHEMA: Running experiment $exp/${#experiment_list[@]} length = $length"
    # echo ${CMD} TEST_ARGS=\"${TEST_ARGS}\"
    # ${CMD} > /dev/null
    ${CMD} &>> $$.log
    # EXIT_CODE=$?; check_exit_code "$EXIT_CODE" "$CMD" && return $EXIT_CODE

    ###################
    # Post-processing #
    ###################
    # Parse simulation data
    if [[ "${MAKE_TEST_TARGET}" == "test_asp_fpga_sim" ]]; then
        ${ROOT_DIR}/scripts/parse_simulation_data_json.sh       \
            ${SYCL_ASP_BUILD_DIR}/${SYCL_IP_NAME}.fpga_sim.prj  \
            ${out_dir}/cycles_${RS_SCHEMA}.csv                  \
            ${length}
    fi
done

############
# Clean up #
############
rm $exp_file