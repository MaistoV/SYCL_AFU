# Same user, same VF
# RESULT: Fails
NUM_THREADS=9
for (( i=1; i<=$NUM_THREADS; i++ ))
do
    echo "Running $i"
    # Launch in background
    host_exerciser lpbk &
    pid_array[${i}]=$!
    echo "Launching PID ${pid_array[${i}]}"
done

# Wait for all children
# wait

# wait for all pids and save exit codes
for (( i=1; i<=$NUM_THREADS; i++ ))
# for pid in ${pid_array[*]};
do
    echo "Waiting PID=${pid_array[${i}]}"
    wait ${pid_array[${i}]}
    exit_code_array[${i}]=$?
done

# Check exit codes
echo "PID, Exit code"
for (( i=1; i<=$NUM_THREADS; i++ ));
do 
    echo "${pid_array[${i}]}: ${exit_code_array[${i}]}"
done


