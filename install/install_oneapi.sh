source $ROOT_DIR/install/install_settings.sh

# Prerequisites
# sudo dnf install numactl-devel ncurses-compat-libs
sudo apt install numactl ncurses-base # ? 

# Downloads
mkdir -p $DOWNLOADS_DIR
cd $DOWNLOADS_DIR

# Installer
if [ ! -e $DOWNLOADS_DIR/l_BaseKit_p_2024.0.1.46.sh ]; then
    wget https://registrationcenter-download.intel.com/akdlm/IRC_NAS/163da6e4-56eb-4948-aba3-debcec61c064/l_BaseKit_p_2024.0.1.46.sh
fi
sudo sh ./l_BaseKit_p_2024.0.1.46.sh

# Quartus patch
IOFS_PATCH=quartus-0.0-0.02iofs-linux.run
if [ ! -e $DOWNLOADS_DIR/$IOFS_PATCH ]; then
    ln -s $HTS_FIM_RELEASE/ofs-agx7-pcie-attach/license/quartus-0.0-0.02iofs-linux.run
fi
# Install patch
./$IOFS_PATCH          \
    --mode unattended  \ 
    --accept_eula 1    \
    --installdir $QUARTUS_INSTALL_DIR

cd $OFS_ASP_ROOT
./scripts/build-bsp.sh

cd $ROOT_DIR




