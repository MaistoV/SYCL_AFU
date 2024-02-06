source ubuntu_install_settings.sh

# Install prerequisites (patched for Ubuntu 22.04)
sudo apt install -y python3 python3-pip python3-dev \
    gdb vim git gcc g++ make cmake libuuid1 libsystemd-dev sudo nmap \
    python3-jsonschema libjson-c-dev libtbb-dev libcap-dev \
    libspdlog-dev libcli11-dev python3-pyyaml-env-tag python3-pybind11 \
    libhwloc-dev libedit-dev build-essential flex bison libelf-dev libssl-dev \
    libncurses-dev libssl-dev libelf-dev 
# librpm-dev
#   kernel-headers kernel-devel

# $ wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/pybind11-devel-2.4.3-2.el8.x86_64.rpm

# $ wget http://ftp.pbone.net/mirror/archive.fedoraproject.org/epel/8.4/Everything/x86_64/Packages/p/python3-pybind11-2.4.3-2.el8.x86_64.rpm

# $ sudo dnf localinstall ./python3-pybind11-2.4.3-2.el8.x86_64.rpm ./pybind11-devel-2.4.3-2.el8.x86_64.rpm -y

python3 -m pip install --user jsonschema virtualenv pudb pyyaml
sudo pip3 uninstall setuptools
sudo pip3 install Pybind11==2.10.0
sudo pip3 install setuptools==59.6.0 --prefix=/usr

# Clone kernel
mkdir -p $UTILS_BUILD_DIR
cd $UTILS_BUILD_DIR
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
# Patches for Ubuntu
# Disable modules signature
# NOTE: this is only meant for developement, and not ment for production as it may incurr in a security hole
sed -i -r 's/CONFIG_SYSTEM_REVOCATION_KEYS=.*/CONFIG_SYSTEM_REVOCATION_KEYS=""/' .config

# Build kernel and modules
make olddefconfig
time make -j `nproc`
time make -j `nproc` modules

# Install modules from sources
time sudo make -j `nproc` modules_install
time sudo make -j `nproc` install

# Install modules from packages
# NOTE: UNTESTED
# make INSTALL_MOD_STRIP=1 bindeb-pkg
# cd ~/rpmbuild/RPMS/x86_64
# sudo rpm -i kernel*.rpm

# Update grub boot config
# Add following selection string to /etc/default/grub:GRUB_CMDLINE_LINUX
# NOTE: this assumes GRUB_CMDLINE_LINUX to be empty
sudo sed "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200\"/g" /etc/default/grub

# Update default grub entry
# TODO: change with new kernel name
sudo sed -i "s/GRUB_DEFAULT=.+/GRUB_DEFAULT=\"Advanced options for Ubuntu>Ubuntu, with Linux 5.15.52-dfl\"/g" /etc/default/grub
sudo update-grub

# Reboot system and check installed drivers
sudo reboot