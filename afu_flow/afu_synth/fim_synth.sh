#!/bin/bash

# Setup
#source $IOFS_BUILD_ROOT/../afu_synth/settings_synth.sh
source settings_hitek.sh

# Build FIM
cd $OFS_ROOTDIR

# BUILD_TARGET=n6000/base_x16/adp  # includes the wrong pcie_ss.v file, with the wrong port definition
BUILD_TARGET=ofs_hpc/base_x16/htk_lp_pcie
# BUILD_TARGET=ofs_hpc/base_x16/adp

#./syn/build_top.sh ofs_hpc/base_x16/htk_lp_pcie work_x16_htk_lp_pcie

WORK_DIR=work_x16_htk_lp_pcie

## Build FIM and PR-tree
# ./syn/build_top.sh -p $BUILD_TARGET $WORK_DIR

## Build FIM without PR-tree
## and build PR-tree separetely
./syn/build_top.sh $BUILD_TARGET $WORK_DIR 
# && \
# ./syn/common/scripts/generate_pr_release.sh

