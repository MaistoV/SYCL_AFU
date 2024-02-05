#!/bin/bash

# Setup
unset DISPLAY # this is necessary to have Java run headless
#source settings_hitek.sh
source settings_synth.sh

# Build FIM
cd $OFS_ROOTDIR

export BUILD_TARGET=ofs_hpc/base_x16/htk_lp_pcie

#./syn/build_top.sh ofs_hpc/base_x16/htk_lp_pcie work_x16_htk_lp_pcie

export FIM_BUILD_DIR=work_x16_htk_lp_pcie_pr

## Build FIM and PR-tree
./syn/build_top.sh -p $BUILD_TARGET $FIM_BUILD_DIR

## Build FIM without PR-tree
## and build PR-tree separetely
#./syn/build_top.sh $BUILD_TARGET $FIM_BUILD_DIR
# && \
# ./syn/common/scripts/generate_pr_release.sh \
# 	-t pr_release/ \
# 	ofs_hpc/base_x16/htk_lp_pcie \
# 	work_x16_htk_lp_pcie
