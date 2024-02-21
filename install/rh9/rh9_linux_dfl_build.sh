source $ROOT_DIR/install/install_settings.sh

# Install prerequisites

# NOT COMPATIBLE WITH release 9
# sudo subscription-manager release --set=8.6
# sudo dnf update
# sudo subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms

# Try with 9
sudo subscription-manager repos --enable codeready-builder-for-rhel-9-x86_64-rpms
sudo dnf install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-9.noarch.rpm

sudo dnf install -y python3 python3-pip python3-devel \
    gdb vim git gcc gcc-c++ make cmake libuuid-devel rpm-build systemd-devel nmap \
    python3-jsonschema json-c-devel tbb-devel rpmdevtools libcap-devel \
    python3-pyyaml hwloc-devel libedit-devel git kernel-headers kernel-devel elfutils-libelf-devel ncurses-devel openssl-devel bison flex cli11-devel spdlog-devel

# Stup python
python3 -m pip install --user jsonschema virtualenv pudb pyyaml
sudo pip3 uninstall setuptools
sudo pip3 install Pybind11==2.10.0 --proxy http://yourproxy:xxx
sudo pip3 install setuptools==59.6.0 --prefix=/usr --proxy http://yourproxy:xxx

# Download packages
mkdir -p $DOWNLOADS_DIR
cd $DOWNLOADS_DIR
wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/pybind11-devel-2.4.3-2.el8.x86_64.rpm
wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/python3-pybind11-2.4.3-2.el8.x86_64.rpm
sudo dnf localinstall ./python3-pybind11-2.4.3-2.el8.x86_64.rpm ./pybind11-devel-2.4.3-2.el8.x86_64.rpm -y
cd $ROOT_DIR

# Clone kernel
mkdir -p $INSTALL_BUILD_DIR
cd $INSTALL_BUILD_DIR
# clone Linux DFL repo from github
git clone https://github.com/OFS/linux-dfl.git
cd linux-dfl
# checkout Linux DFL tag ofs-2023.2-6.1-1
git checkout tags/ofs-2023.2-6.1-1 -b nc220 
git describe --tags
echo "Expected ofs-2023.2-6.1-1"
git apply $HTS_PATCHES_DIR/linux-dfl-nc220-flashv1.patch
# Note: 
# For fpga flash layout V3 or V2, use following commands
# git apply <path-to-patches-dir>/linux-dfl-nc220-flashv3.patch
# -or-
# git apply <path-to-patches-dir>/linux-dfl-nc220-flashv2.patch

# Configure build
cd $UTILS_BUILD_DIRlinux-dfl
cp /boot/config-`uname -r` .config
cat configs/dfl-config >> .config
echo 'CONFIG_LOCALVERSION="-dfl"' >> .config
echo 'CONFIG_LOCALVERSION_AUTO=y' >> .config
sed -i -r 's/CONFIG_SYSTEM_TRUSTED_KEYS=.*/CONFIG_SYSTEM_TRUSTED_KEYS=""/' .config
sed -i '/^CONFIG_DEBUG_INFO_BTF/ s/./#&/' .config
echo 'CONFIG_DEBUG_ATOMIC_SLEEP=y' >> .config
export LOCALVERSION=

# Build kernel and modules
make olddefconfig
time make -j `nproc`
time make -j `nproc` modules

# Install RPMs
make INSTALL_MOD_STRIP=1 binrpm-pkg
cd ~/rpmbuild/RPMS/x86_64
sudo rpm -Uvh --oldpackage kernel-*.rpm

# Package
PACKAGE_DIR=$INSTALL_BUILD_DIR/package/rpm/linux-dfl
mkdir -p $PACKAGE_DIR
cp kernel-*.rpm $PACKAGE_DIR

# Update grub boot config
# Add following selection string to /etc/default/grub:GRUB_CMDLINE_LINUX
sudo sed -i 's/GRUB_CMDLINE_LINUX="[^"]*/& intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200/' /etc/default/grub
# sudo grub2-mkconfig
# sudo grub2-mkconfig -o /boot/grub2/grub.cfg
# grubby --update-kernel=/vmlinuz-6.1.41-dfl --args=GRUB_CMDLINE_LINUX="crashkernel=1G-4G:192M,4G-64G:256M,64G-:512M resume=/dev/mapper/rhel_rh9-swap rd.lvm.lv=rhel_rh9/root rd.lvm.lv=rhel_rh9/swap rhgb quiet intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200"

sudo grub2-mkconfig --update-bls-cmdline


echo "Reboot system and check:"
echo "  1. Installed DLF drivers:   lsmod | grep dfl"
echo "  2. Boot arguments       :   cat /proc/cmdline"
