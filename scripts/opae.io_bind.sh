#!/bin/bash

# Initial setup
SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
echo "[INFO] Found $SRIOV_NUMVF sriov_numvfs"
if [ $SRIOV_NUMVF == "0" ]; then
    echo "[INFO] Setting up ${FIM_NUM_PF0_VFS} VFs"
    
    PCIE_BDF=$(lspci | grep bcce | head -n1 | awk '{print $1}')
    # Create new VFs ( XXXX:XX:XX.5-...)
    sudo pci_device ${PCIE_BDF} vf ${FIM_NUM_PF0_VFS}

    # FIM and PR AFUs VFs
    # Exclude PF0.VF0
    OPAEIO_SDBFs=$( opae.io ls | grep -v 01:00.0 | awk '{print $1}' | sed -E "s/(\[|\])//g" )
    for sbdf in ${OPAEIO_SDBFs}; do
        sudo opae.io init -d $sbdf $USER:$USER
    done

    SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
    echo "[INFO] Setup $SRIOV_NUMVF sriov_numvfs"
    echo "[INFO] Running fpgainfo port..."
    echo "[INFO] See fpgainfo_port.new.log"
fi

sudo chown $USER /etc/opae/