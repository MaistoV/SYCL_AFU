#!/bin/bash

REQUESTED_VFs=100
if [[ $1 != "" ]]; then
    REQUESTED_VFs=$1
fi
# Initial setup
SRIOV_NUMVF=$(sudo find /sys/ -wholename "*/${PAC_PCIE_SBD}.0/sriov_numvfs" | head -n1 | xargs cat)
SRIOV_TOTVF=$(sudo find /sys/ -wholename "*/${PAC_PCIE_SBD}.0/sriov_totalvfs" | head -n1 | xargs cat)
echo "[INFO] Found $SRIOV_NUMVF sriov_numvfs"
echo "[INFO] Found $SRIOV_TOTVF sriov_totalvfs"
echo "[INFO] Requested $REQUESTED_VFs VFs"
# Min(REQUESTED_VFs, SRIOV_TOTVF)
TARGET_VFs=$REQUESTED_VFs
if [ $TARGET_VFs -gt $SRIOV_TOTVF ]; then
    TARGET_VFs=$SRIOV_TOTVF
fi

# Setup TARGET_VFs
if [ $TARGET_VFs -gt 0 ]; then
    echo "[INFO] Setting up ${TARGET_VFs} VFs"
    
    # Create new VFs
    sudo pci_device ${PAC_PCIE_BD}.0 vf ${TARGET_VFs}

    # FIM and PR AFUs VFs
    # Exclude PF0.VF0 (B:00.0)
    OPAEIO_SDBFs=$( opae.io ls | sort | grep -v ${PAC_PCIE_SBD}.0 | awk '{print $1}' | sed -E "s/(\[|\])//g" )
    for sbdf in ${OPAEIO_SDBFs}; do
        sudo opae.io init -d $sbdf $USER:$USER
    done

    SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | grep ${PAC_PCIE_SBD} | xargs cat)
    echo "[INFO] Setup $SRIOV_NUMVF sriov_numvfs"
fi

sudo chown $USER /etc/opae/