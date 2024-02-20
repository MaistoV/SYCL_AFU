source ../patches_install_settings.sh

# Prerequisites (same as Linux DFL?)
sudo apt-get install -y python3 python3-pip python3-jsonschema  python3-dev git gcc g++ make cmake uuid-dev \
    libjson-c-dev libhwloc-dev libtbb-dev libedit-dev libudev-dev linuxptp pandoc \
    devscripts debhelper doxygen libcli11-dev libspdlog-dev libsystemd-dev libcap-dev python3-pyyaml-env-tag
# librpm-dev python3-sphinx  python3-virtualenv  podman

pip3 install jsonschema virtualenv pyyaml pybind11
# pip3 install --upgrade --prefix=/usr pip setuptools pybind11
# sudo pip3 install Pybind11==2.10.0
# sudo pip3 install setuptools==59.6.0 --prefix=/usr

# Clone
mkdir -p $INSTALL_BUILD_DIR
cd $INSTALL_BUILD_DIR
git clone https://github.com/OFS/opae-sdk.git
cd opae-sdk
git checkout tags/2.8.0-1 -b nc220 
git describe --tags
echo "Expected 2.8.0-1"
git apply $HTS_PATCHES_DIR/opae-sdk-nc220.patch

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