#!/bin/bash

# Initial setup
SRIOV_NUMVF=$(sudo find /sys/ -wholename "*/${PAC_PCIE_SBD}.0/sriov_numvfs" | head -n1 | xargs cat)
SRIOV_TOTVF=$(sudo find /sys/ -wholename "*/${PAC_PCIE_SBD}.0/sriov_totalvfs" | head -n1 | xargs cat)
echo "[INFO] Found $SRIOV_NUMVF sriov_numvfs"
echo "[INFO] Found $SRIOV_TOTVF sriov_totalvfs"
if [ $SRIOV_NUMVF -lt $SRIOV_TOTVF ]; then
    echo "[INFO] Setting up ${FIM_NUM_PF0_VFS} VFs"
    
    # Create new VFs
    sudo pci_device ${PAC_PCIE_BD}.0 vf ${FIM_NUM_PF0_VFS}

    # FIM and PR AFUs VFs
    # Exclude PF0.VF0 (B:00.0)
    OPAEIO_SDBFs=$( opae.io ls | grep -v ${PAC_PCIE_SBD}.0 | awk '{print $1}' | sed -E "s/(\[|\])//g" )
    for sbdf in ${OPAEIO_SDBFs}; do
        sudo opae.io init -d $sbdf $USER:$USER
    done

    SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
    echo "[INFO] Setup $SRIOV_NUMVF sriov_numvfs"
    echo "[INFO] Running fpgainfo port..."
    echo "[INFO] See fpgainfo_port.new.log"
fi

sudo chown $USER /etc/opae/