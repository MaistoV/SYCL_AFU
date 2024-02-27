source $ROOT_DIR/install/install_settings.sh

source $ROOT_DIR/install/rh8_9/rh8_9_prerequisites.sh

# OPAE-SDK

# Clear any old installation
cd $ROOT_DIR
sudo dnf remove opae* -y

cd $PACKAGE_DIR/rpm/opae-sdk
sudo dnf install -y opae*.rpm
rpm -qa | grep opae

# Linux DFL
cd $PACKAGE_DIR/rpm/linux-dfl

sudo dnf localinstall kernel-*.rpm

sudo sed -i 's/GRUB_CMDLINE_LINUX="[^"]*/& intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200/' /etc/default/grub
sudo grub2-mkconfig

echo "Reboot system and check:"
echo "  1. Installed DLF drivers:   lsmod | grep dfl"
echo "  2. Boot arguments       :   cat /proc/cmdline"

cd $ROOT_DIR
