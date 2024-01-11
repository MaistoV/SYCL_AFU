CMD=$1
FLAGS=$2
SUBCMD=$3

PROLOGUE=[$SUBCMD]

NUM_REPS=3

for (( i=1; i<=$NUM_REPS; i++ ))
do
    echo "$PROLOGUE Running iteration $i"
    # Launch in background
    $CMD $FLAGS $SUBCMD > /dev/null&
    pid_array[${i}]=$!
    echo "$PROLOGUE Launched PID ${pid_array[${i}]}"
    # Wait for child
    wait ${pid_array[${i}]}
    exit_code_array[${i}]=$?
done

# Check exit codes
for (( i=1; i<=$NUM_REPS; i++ ));
do 
    if [[ "0" != ${exit_code_array[${i}]} ]];
    then
        echo "$PROLOGUE Non-zero exit code: PID ${pid_array[${i}]}, Exit code ${exit_code_array[${i}]}"
    fi
done


