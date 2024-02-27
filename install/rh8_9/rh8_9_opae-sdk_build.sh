source $ROOT_DIR/install/install_settings.sh
source $ROOT_DIR/install/rh8_9/rh8_9_prerequisites.sh

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
podman run -ti -v "$PWD":/src:Z                 \
    -v "$PWD/../rh8_9/":/rh8_9:Z                \
    -w /src registry.access.redhat.com/ubi8:8.6 \
    # bash /rh8_9/rh8_9_opae-sdk_container.sh 

# On exit from container

# Install
cd $INSTALL_BUILD_DIR/opae-sdk/packaging/opae/rpm
rm -rf opae-2.8.0-1.el8.src.rpm
# sudo dnf localinstall -y opae*.rpm # Direct install

# Package
OPAE_SDK_PACKAGE_DIR=$PACKAGE_DIR/rpm/opae-sdk
mkdir -p $OPAE_SDK_PACKAGE_DIR
cp opae*.rpm $OPAE_SDK_PACKAGE_DIR
 