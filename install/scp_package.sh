declare -a addresses=( rh8-53 rh8-54 rh8-55 rh8-56 rh8-57 rh8-58 rh8-59 )

for ip in ${addresses[@]}; do
    echo "[INFO] SCPing to $ip"
    ssh vmaisto@$ip mkdir -p $PACKAGE_DIR
    scp -pr $PACKAGE_DIR vmaisto@$ip:$(dirname $PACKAGE_DIR)
done