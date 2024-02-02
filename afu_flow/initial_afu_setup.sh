#!/bin/bash
# This scripts downloads the necessary components for AFU developement flow
#   1. FIM sources <https://github.com/otcshare/intel-ofs-fim.git>
#       * Hitek's release points to SFTP 2.2-beta
#   2. Basic Building Blocks <https://github.com/OPAE/ofs-platform-afu-bbb.git>
#   3. Example AFUs <https://github.com/OPAE/intel-fpga-bbb.git>
#       * New release points to <https://github.com/OFS/examples-afu>
#   4. ASE sources <https://github.com/OFS/opae-sim>

source settings_afu.sh

# OFS working dir
mkdir -p build
export IOFS_BUILD_ROOT=$PWD/build
echo "[INFO] Building in $IOFS_BUILD_ROOT"

#########################
# 1. Download/Clone FIM #
#########################

rm -rf $OFS_ROOTDIR
## Copy from SFTP
# export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/intel-ofs-fim-ofs-2.2.0-beta
# export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/intel-ofs-fim_20220728
# export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/older_intel-ofs-fim_2.2.0-beta_AGFB014R24BE2V_20230127
export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/newer_intel-ofs-fim_2.2.0-beta_AGFB014R24AE2V_20230127

## Clone from GitHub (actually from a zip archive)
# export OTCSHARE_INTEL_OFS_FIM=$IOFS_BUILD_ROOT/../../otcshare_dumps/otcshare_22_Nov_2023/intel-ofs-fim-ofs-2.2.0-beta
# # git clone https://github.com/otcshare/intel-ofs-fim.git
# # cd intel-ofs-fim
# # git checkout tags/ofs-2.3.0

echo "[INFO] Getting FIM from $OTCSHARE_INTEL_OFS_FIM"
cp -r $OTCSHARE_INTEL_OFS_FIM $OFS_ROOTDIR
cd $OTCSHARE_INTEL_OFS_FIM

###########################################
# 2. Download Basic Building Blocks (BBB) #
###########################################
cd $IOFS_BUILD_ROOT

# AFU_BBB_CHECKOUT=ofs-2023.3-1-rc2 # 14 Oct 2023
AFU_BBB_CHECKOUT=ofs-2023.2-1     # 30 Aug 2023
# AFU_BBB_CHECKOUT=ofs-2023.1-1     # 14 Apr 2023

echo "[INFO] Getting BBB from OPAE's GitHub"
cd $IOFS_BUILD_ROOT
if [ ! -d ofs-platform-afu-bbb ]; then
    git clone https://github.com/OPAE/ofs-platform-afu-bbb.git
fi
cd ofs-platform-afu-bbb
git checkout $AFU_BBB_CHECKOUT
cd ..

##########################################
# 3. Clone the intel-fpga-bbb repository #
# 3. Example AFUs                        #
##########################################
cd $IOFS_BUILD_ROOT

INTE_FPGA_BBB_CHECKOUT=ofs-2023.2-1 #  4 Aug 2023
# INTE_FPGA_BBB_CHECKOUT=1.3.0-1    # 18 Jan 2019

echo "[INFO] Getting example AFUs from OPAE's GitHub"
cd $IOFS_BUILD_ROOT 
if [ ! -d intel-fpga-bbb ]; then
    git clone https://github.com/OPAE/intel-fpga-bbb.git
fi
cd intel-fpga-bbb
git checkout $INTE_FPGA_BBB_CHECKOUT
cd ..

#############
# Build FIM #
#############
cd $IOFS_BUILD_ROOT
source ../afu_synth/fim_synth.sh

# Download FIM to FPGA
# sudo fpgasupdate ofs_top_page1_unsigned_user1.bin <N6001 SKU2 PCIe b:d.f>
# sudo fpgasupdate ofs_top_page2_unsigned_user2.bin <N6001 SKU2 PCIe b:d.f>
# sudo rsu fpga --page=user1 <N6001 SKU2 PCIe b:d.f>

#########################
# 4. Clone ad build ASE #
#########################
cd $IOFS_BUILD_ROOT

# Clone and build ASE
echo "[INFO] Getting ASE"
if [ ! -d opae-sim ]; then
    git clone https://github.com/OPAE/opae-sim.git
    cd opae-sim  
    # Releases compatible with NC100, OFS 2.2-beta
    git checkout tags/2.1.2-4 -b release/2.1.2
    # for mock/opae_std.h 
    # export C_INCLUDE_PATH="/usr/src/debug/opae-2.1.1-1.el8.x86_64/tests/framework"
    mkdir build
    cd build
    # point to opae-config.cmake
    export opae_DIR=~//Intel_OFS/opae-sdk-2.1.1/packaging/opae/deb/opae-2.1.1/debian/tmp/usr/lib/opae-2.1.1/
    cmake -DCMAKE_INSTALL_PREFIX=/usr ..
    make
    sudo make install  
fi

# cd to working directory again
cd $IOFS_BUILD_ROOT
