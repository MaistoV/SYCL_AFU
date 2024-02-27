source $ROOT_DIR/install/install_settings.sh

source $ROOT_DIR/install/rh8_9/rh8_9_prerequisites.sh

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

# Build RPMs
make INSTALL_MOD_STRIP=1 binrpm-pkg
cd ~/rpmbuild/RPMS/x86_64
# sudo rpm -Uvh kernel-*.rpm # Direct install

# Package
LINUX_PACKAGE_DIR=$PACKAGE_DIR/rpm/linux-dfl
mkdir -p $LINUX_PACKAGE_DIR
cp kernel-*.rpm $LINUX_PACKAGE_DIR