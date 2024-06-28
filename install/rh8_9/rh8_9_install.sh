# This script direactory
SCRIPT_DIR=$( dirname $( realpath ${BASH_SOURCE[0]} ) )
PACKAGE_DIR=$SCRIPT_DIR/package

#################
# Prerequisites #
#################
source $PWD/rh8_9_prerequisites.sh

#############
# Linux DFL #
#############
cd $PACKAGE_DIR/rpm/linux-dfl

sudo dnf localinstall -y kernel-*.rpm

# NOTE: We need a Gigapage for each VFProxy
sudo sed -i 's/GRUB_CMDLINE_LINUX="[^"]*/& intel_iommu=on pcie=realloc default_hugepagesz=2MB hugepagesz=1G hugepages=6 hugepagesz=2M hugepages=200/' /etc/default/grub
sudo grub2-mkconfig > /dev/null

echo "Reboot system and check:"
echo "  1. Booted kernel        :   uname -r"
echo "  2. Installed DLF drivers:   lsmod | grep dfl"
echo "  3. Boot arguments       :   cat /proc/cmdline"

############
# OPAE-SDK #
############

# Clear any old installation
cd $HOME # Move to ove to avoid aliasing in stdin
sudo dnf remove opae* -y

cd $PACKAGE_DIR/rpm/opae-sdk
sudo dnf install -y opae*.rpm
rpm -qa | grep opae

cd $PACKAGE_DIR/..
