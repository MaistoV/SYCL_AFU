source ../patches_install_settings.sh

# Clone
mkdir -p $INSTALL_BUILD_DIR
cd $INSTALL_BUILD_DIR
git clone https://github.com/OFS/opae-sdk.git
cd opae-sdk
git checkout tags/2.8.0-1 -b nc220 
git describe --tags
echo "Expected 2.8.0-1"
git apply $HTS_PATCHES_DIR/opae-sdk-nc220.patch

# Pull and launch container
podman pull registry.access.redhat.com/ubi8:8.6
podman run -ti -v "$PWD":/src:Z -w /src registry.access.redhat.com/ubi8:8.6

# Prerequisites (same as Linux DFL?)

# Build with Cmake
# cd $INSTALL_BUILD_DIR/opae-sdk
# mkdir build
# cd build/
# # cmake .. -DCPACK_GENERATOR=DEB -DOPAE_BUILD_FPGABIST=ON
# make -j `nproc`
# # NOTE: Install through dpkg so that you can control the installation more easily
# make -j `nproc` package_deb
# sudo dpkg -i opae*.deb

# Build by script
cd $INSTALL_BUILD_DIR/opae-sdk/packaging/opae/deb
./create

# Install with dpkg
# NOTE: Install through dpkg so that you can control the installation more easily
sudo dpkg -i opae*.deb

# Check installed
apt list opae*