source install_settings.sh

declare -a addresses=( rh8-51 rh8-52 rh8-53 rh8-54 rh8-55 rh8-56 rh8-57 rh8-58 rh8-59 )

for ip in ${addresses[@]}; do
    echo "[INFO] SCPing to $ip"
    ssh $USER@$ip mkdir -p $HOME/install/
    scp -pr $PACKAGE_DIR rh8_9/rh8_9_install.sh rh8_9/rh8_9_prerequisites.sh check_install.sh $USER@$ip:$HOME/install/
done
