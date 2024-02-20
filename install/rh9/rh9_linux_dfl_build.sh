source ../patches_install_settings.sh

# Install prerequisitessudo dnf update
subscription-manager repos --enable codeready-builder-for-rhel-8-x86_64-rpms
sudo dnf install https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm

sudo dnf install -y python3 python3-pip python3-devel \
gdb vim git gcc gcc-c++ make cmake libuuid-devel rpm-build systemd-devel nmap \
python3-jsonschema json-c-devel tbb-devel rpmdevtools libcap-devel \
python3-pyyaml hwloc-devel libedit-devel git kernel-headers kernel-devel elfutils-libelf-devel ncurses-devel openssl-devel bison flex cli11-devel spdlog-devel

python3 -m pip install --user jsonschema virtualenv pudb pyyaml

sudo pip3 uninstall setuptools

sudo pip3 install Pybind11==2.10.0 --proxy http://yourproxy:xxx

sudo pip3 install setuptools==59.6.0 --prefix=/usr --proxy http://yourproxy:xxx

wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/pybind11-devel-2.4.3-2.el8.x86_64.rpm

wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/python3-pybind11-2.4.3-2.el8.x86_64.rpm

sudo dnf localinstall ./python3-pybind11-2.4.3-2.el8.x86_64.rpm ./pybind11-devel-2.4.3-2.el8.x86_64.rpm -y

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

# Install modules from sources
# time sudo make -j `nproc` modules_install
# time sudo make -j `nproc` install

# Install modules from packages
# NOTE: UNTESTED
make INSTALL_MOD_STRIP=1 bindeb-pkg
cd ~/rpmbuild/RPMS/x86_64
sudo rpm -i kernel*.rpm

# Update grub boot config
# Add following selection string to /etc/default/grub:GRUB_CMDLINE_LINUX
# NOTE: this assumes GRUB_CMDLINE_LINUX to be empty, hence it may only work only the first time you set up a system
sudo sed "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200\"/g" /etc/default/grub

# Update default grub entry
# TODO: change with new kernel name
sudo sed -i "s/GRUB_DEFAULT=.+/GRUB_DEFAULT=\"Advanced options for Ubuntu>Ubuntu, with Linux 6.1.41-dfl-dirty\"/g" /etc/default/grub
sudo update-grub

# Reboot system and check installed drivers
sudo reboot