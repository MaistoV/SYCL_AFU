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
# podman pull registry.access.redhat.com/ubi8:8.6
# podman run -ti -v "$PWD":/src:Z -w /src registry.access.redhat.com/ubi8:8.6


sudo dnf install --enablerepo=codeready-builder-for-rhel-9-x86_64-rpms -y python3 \
    python3-pip python3-devel python3-jsonschema python3-pyyaml git gcc gcc-c++ make \
    cmake libuuid-devel json-c-devel hwloc-devel tbb-devel cli11-devel spdlog-devel \
    libedit-devel systemd-devel doxygen python3-sphinx pandoc rpm-build rpmdevtools\
    python3-virtualenv yaml-cpp-devel libudev-devel libcap-devel

# Conflicts with pyhton3.9
# Install in $USER/.local
pip3 install --upgrade --user pip setuptools pybind11 

# Build
./packaging/opae/rpm/create unrestricted

# Install
cd opae-sdk/packaging/opae/rpm
rm -rf opae-2.8.0-1.el9.src.rpm
sudo dnf localinstall -y opae*.rpm

# Check installed
rpm -qa | grep opae