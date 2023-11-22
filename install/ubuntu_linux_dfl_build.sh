# Set working directory if not already set
: ${WORK_DIR=/home/intelFPGA}

# Install prerequisites (patched for Ubuntu, kernel 5.15)
sudo apt install -y python3 python3-pip python3-dev \
gdb vim git gcc g++ make cmake libuuid1 libsystemd-dev sudo nmap \
python3-jsonschema libjson-c-dev libtbb-dev  libcap-dev \
libspdlog-dev libcli11-dev python3-pyyaml-env-tag python3-pybind11 \
libhwloc-dev libedit-dev build-essential flex bison libelf-dev libssl-dev

python3 -m pip install --user jsonschema virtualenv pudb pyyaml

sudo pip3 uninstall setuptools

sudo pip3 install Pybind11==2.10.0

sudo pip3 install setuptools==59.6.0 --prefix=/usr

# Clone kernel
mkdir -p $WORK_DIR/Intel_OFS/
cd $WORK_DIR/Intel_OFS/
git init
git clone https://github.com/OPAE/linux-dfl.git
cd $WORK_DIR/Intel_OFS/linux-dfl
git checkout tags/ofs-2022.2-1 -b fpga-ofs-dev-5.15-lts
git describe --match ofs-2022.2-1
git branch
echo "Expected fpga-ofs-dev-5.15-lts"

# Configure build
cp /boot/config-`uname -r` .config
cat configs/dfl-config >> .config
echo 'CONFIG_LOCALVERSION="-dfl"' >> .config
echo 'CONFIG_LOCALVERSION_AUTO=y' >> .config
sed -i -r 's/CONFIG_SYSTEM_TRUSTED_KEYS=.*/CONFIG_SYSTEM_TRUSTED_KEYS=""/' .config
sed -i '/^CONFIG_DEBUG_INFO_BTF/ s/./#&/' .config
echo 'CONFIG_DEBUG_ATOMIC_SLEEP=y' >> .config
export LOCALVERSION=
# Patches for Ubuntu
#sed -i 's/CONFIG_X86_X32=y/CONFIG_X86_X32=n/g' .config
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
#make INSTALL_MOD_STRIP=1 bindeb-pkg
#cd ~/rpmbuild/RPMS/x86_64
#sudo rpm -i kernel*.rpm

# Update grub boot config
# Add following selection string to /etc/default/grub:GRUB_CMDLINE_LINUX
# NOTE: this assumes GRUB_CMDLINE_LINUX to be empty
sudo sed -i "s/GRUB_CMDLINE_LINUX=\"\"/GRUB_CMDLINE_LINUX=\"intel_iommu=on pcie=realloc hugepagesz=2M hugepages=200\"/g" /etc/default/grub

# Update default grub entry
sudo sed -i "s/GRUB_DEFAULT=0/GRUB_DEFAULT=\"Advanced options for Ubuntu>Ubuntu, with Linux 5.15.52-dfl\"/g" /etc/default/grub
sudo update-grub

# Reboot system and check installed drivers
sudo reboot