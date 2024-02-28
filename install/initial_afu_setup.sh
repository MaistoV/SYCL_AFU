#!/bin/bash
# This scripts downloads the necessary components for AFU developement flow
#   1. FIM sources <https://github.com/OFS/ofs-agx7-pcie-attach.git>
#       * Hitek's release points 489c64187887fc7a58c5d04c3be968bef2bde8c6 [HEAD -> release/ofs-2023.2, tag: ofs-2023.2-1]
#   2. OPAE-SDK <https://github.com/OFS/ofs-platform-afu-bbb.git> tag: ofs-2023.2-1
#   3. External repositories
#       * <https://github.com/OPAE/ofs-platform-afu-bbb.git>
#       * <https://github.com/OPAE/intel-fpga-bbb.git>
#       * <https://github.com/OFS/examples-afu>
#   4. ASE sources <https://github.com/OFS/opae-sim>
#   5. Buid FIM

source settings_afu.sh

# OFS working dir
export IOFS_BUILD_ROOT=$HTS_RELEASE
export EXTERNAL_CLONE_DIR=$IOFS_BUILD_ROOT/ofs-agx7-pcie-attach/external
mkdir -p build
echo "[INFO] IOFS_BUILD_ROOT in $IOFS_BUILD_ROOT"

#########################
# 1. Download/Clone FIM #
#########################
# rm -rf $OFS_ROOTDIR
# # Copy from SFTP
# export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/newer_intel-ofs-fim_2.2.0-beta_AGFB014R24AE2V_20230127
# echo "[INFO] Getting FIM from $OTCSHARE_INTEL_OFS_FIM"
# cp -r $OTCSHARE_INTEL_OFS_FIM $OFS_ROOTDIR

## Clone from GitHub
# # git clone https://github.com/otcshare/ofs-agx7-pcie-attach.git
# # cd /ofs-agx7-pcie-attach
# # git checkout ofs-2023.2-1

###############
# 2. OPAE-SDK #
###############
cd $INSTALL_BUILD_DIR

echo "[INFO] Getting intel-fpga-bbb from OPAE's GitHub"
if [ ! -d opae-sdk ]; then
    source install/ubuntu_opae-sdk_build.sh
fi

#####################
# 3. External repos # 
#####################
# Basic Building Blocks (PIM for AFU)
# Inlcuded in Hitek FIM release under $HTS_RELEASE/ofs-agx7-pcie-attach/external/ofs-platform-afu-bbb/
# echo "[INFO] Getting BBB from OPAE's GitHub"
# cd $EXTERNAL_CLONE_DIR
# if [ ! -d ofs-platform-afu-bbb ]; then
#     git clone https://github.com/OPAE/ofs-platform-afu-bbb.git
# fi
# cd ofs-platform-afu-bbb
# git checkout $AFU_BBB_CHECKOUT

# intel-fpga-bbb
cd $EXTERNAL_CLONE_DIR
INTE_FPGA_BBB_CHECKOUT=ofs-2023.2-1
echo "[INFO] Getting intel-fpga-bbb from OPAE's GitHub"
if [ ! -d intel-fpga-bbb ]; then
    git clone https://github.com/OPAE/intel-fpga-bbb.git
fi
cd intel-fpga-bbb
git checkout $INTE_FPGA_BBB_CHECKOUT

# example-afu
cd $EXTERNAL_CLONE_DIR
EXAMPLES_AFU_CHECKOUT=ofs-2023.2-1
echo "[INFO] Getting intel-fpga-bbb from OPAE's GitHub"
if [ ! -d examples-afu ]; then
    git clone https://github.com/OPAE/examples-afu.git
fi
cd examples-afu
git checkout $EXAMPLES_AFU_CHECKOUT

##########################
# 4. Clone and build ASE #
##########################
cd $INSTALL_BUILD_DIR

# Clone and build ASE
echo "[INFO] Getting ASE"
# NOTE: OPAE-SDK must be already installed and accessible!
if [ ! -d opae-sim ]; then
    git clone https://github.com/OPAE/opae-sim.git
    cd opae-sim  
    # Releases compatible with C220(?)
    git checkout tags/2.8.0-1 -b release/2.8.0
    # for mock/opae_std.h 
    # export C_INCLUDE_PATH="/usr/src/debug/opae-2.1.1-1.el8.x86_64/tests/framework"
    mkdir build
    cd build
    # Default
    # export C_INCLUDE_PATH=/usr/src/debug/opae-2.5.0-3.el8.x86_64/tests/framework
    # point to opae-config.cmake
    export opae_DIR=$INSTALL_BUILD_DIR/opae-sdk/packaging/opae/deb/opae-2.8.0/debian/tmp/usr/lib/opae-2.8.0/
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make -j `nproc`
    sudo make install  
fi


#############
# Build FIM #
#############
echo "To build FIM with PR-tree, you must run:"
echo "    make fim_build_pr"
echo "To flash on board and reboot it:"
echo "    make fim_udpate"
