# Check installed drivers
# After reboot
ls /usr/lib/modules/5.15.*-dfl/kernel/drivers/fpga
lsmod | grep dfl

# Grant access to other users
sudo chmod a+rw /dev/dfl-port.*
sudo chmod a+rw /dev/dfl-fme.*