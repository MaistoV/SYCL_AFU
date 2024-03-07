#!/bin/bash

#######################################
# THIS IS EXTENDED FROM HITEK RELEASE #
# Main differences:
# 1. Always assume no_hssi
# 2. Never PCIE_SMALL
# 3. Extend for custom OFSS flow
# 4. Extend for null HEMs instantiation
#######################################

# Parse ARGS and variables

# Check Hitek variables have been set
FPGA="$BOARD_VAR"
if [ "$FPGA" == "" ]; then
  echo "ERROR: run the setup_env.sh to set environment variables"
  exit -1
fi

# Check quartus binaries are reachable
if ! command -v quartus_sh &> /dev/null ; then
  echo "ERROR: QUARTUS_HOME in setup_env.sh is not correctly set"
  exit -1
fi

# Parse PCIE_SMALL
if [ "$PCIE_SMALL" == "1" ]; then
    echo "[ERROR] PCIE_SMALL option has been removed, assumed 0"
    exit -1
fi

# Parse NO_HSSI
if [ "$NO_HSSI" == "0" ]; then
    echo "[ERROR] NO_HSSI option has been removed, assumed 1"
    exit -1
fi

# Parse argv(2)
if [[ $2 != "" ]]; then
  OFSS_CONFIG=_$2
fi

#---------------------------------------------

ARGS="htk-nc220-${FPGA}"
WORK_DIR="work_htk_nc220_${FPGA}${OFSS_CONFIG}"
OFSS_FILE="ofs-agx7-pcie-attach/tools/ofss_config/htk-nc220-${FPGA}${OFSS_CONFIG}.ofss"

case "$1" in
  --flat)
    BUILD_ARG=""
    ARGS=$ARGS":flat,no_hssi" # Always assume no_hssi
    WORK_DIR=$WORK_DIR"_flat"
    ;;

  --pr)
    BUILD_ARG="-p"
    ARGS=$ARGS":no_hssi" # Always assume no_hssi
    ;;

  *)
    echo "Usage: $0 <--flat|--pr>"
    exit -1
esac

# Parse NULL_HEMS
if [ "$NULL_HEMS" == "1" ]; then
    echo "[INFO] Removing all HEMs, the PFs would still be instantiatied"
    ARGS=$ARGS",null_he_lb,null_he_mem,null_he_mem_tg"
  # - null_he_lb - Replaces the Host Exerciser Loopback (HE_LBK) with he_null .
  # - null_he_mem - Replaces the Host Exerciser Memory (HE_MEM) with he_null.
  # - null_he_mem_tg - Replaces the Host Exerciser Memory Traffic Generator with he_null.    
    WORK_DIR=$WORK_DIR"_NULL_HEMS"
fi

# Launch build
cd $HTS_RELEASE
./ofs-agx7-pcie-attach/ofs-common/scripts/common/syn/build_top.sh \
  $BUILD_ARG \
  --ofss $OFSS_FILE \
  "$ARGS"  \
  "$WORK_DIR"

