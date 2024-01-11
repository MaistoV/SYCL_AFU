# Check installed drivers
# After reboot
# ls /usr/lib/modules/5.15.*-dfl/kernel/drivers/fpga
# lsmod | grep dfl

# Grant access to other users
sudo chmod a+rw /dev/dfl-port.*
sudo chmod a+rw /dev/dfl-fme.*
sudo chmod a+rw /sys/class/fpga_region/*
# sudo sh -c 'echo 4 > /sys/kernel/mm/hugepages/hugepages-1048576kB/nr_hugepages'
