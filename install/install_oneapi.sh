source $ROOT_DIR/install/install_settings.sh

# Prerequisites
# sudo dnf install numactl-devel ncurses-compat-libs
sudo apt install numactl ncurses-base # ? 

# Downloads
mkdir -p $DOWNLOADS_DIR

# Patch HTS release
TARGET_LIBPKG_EDITOR_A=$AFS_ASP_ROOT/common/source/host/lib/libpkg_editor.a
if [ ! -e $TARGET_LIBPKG_EDITOR_A ]; then
    cd $DOWNLOADS_DIR
    git clone https://github.com/OFS/oneapi-asp.git
    cd oneapi-asp
    git checkout tags/ofs-2023.2-1
    cp $DOWNLOADS_DIR/oneapi-asp/common/source/host/lib/libpkg_editor.a $TARGET_LIBPKG_EDITOR_A
fi

# Install OneAPI base toolkit
cd $DOWNLOADS_DIR
if [ ! -e $DOWNLOADS_DIR/l_BaseKit_p_2024.0.1.46.sh ]; then
    wget https://registrationcenter-download.intel.com/akdlm/IRC_NAS/163da6e4-56eb-4948-aba3-debcec61c064/l_BaseKit_p_2024.0.1.46.sh
fi
sudo sh ./l_BaseKit_p_2024.0.1.46.sh

# Quartus patch
IOFS_PATCH=quartus-0.0-0.02iofs-linux.run
if [ ! -e $DOWNLOADS_DIR/$IOFS_PATCH ]; then
    ln -s $HTS_RELEASE/ofs-agx7-pcie-attach/license/quartus-0.0-0.02iofs-linux.run
fi
# Install patch
./$IOFS_PATCH          \
    --mode unattended  \ 
    --accept_eula 1    \
    --installdir $QUARTUS_INSTALL_DIR

# Build BSP
cd $OFS_ASP_ROOT
./scripts/build-bsp.sh

# Check if prerequisites are installed
echo "Check if prerequisites are installed"
echo "  1. Installed DLF drivers:   lsmod | grep dfl"
echo "  2. Boot arguments       :   cat /proc/cmdline"
echo "  2. OPAE-SDK             :   rpm -qa | grep opae"

cd $ROOT_DIR




