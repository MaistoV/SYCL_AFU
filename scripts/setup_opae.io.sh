#!/bin/bash

# Initial setup
SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
echo "[INFO] Found $SRIOV_NUMVF sriov_numvfs"
if [ $SRIOV_NUMVF == "0" ]; then
    echo "[INFO] Setting up 3 VFs"
    
    PCIE_BDF=$(lspci | grep bcce | head -n1 | awk '{print $1}')
    # Create 3 VFs ( XXXX:XX:XX.5-7)
    sudo pci_device  ${PAC_PCIE_SBD} vf 3

    # Bind VFs to user
    # Component 	    VF 	            Accelerator GUID
    # base PF       	XXXX:XX:XX.0	N/A
    # VirtIO Stub      	XXXX:XX:XX.1	3e7b60a0-df2d-4850-aa31-f54a3e403501
    # HE-MEM Stub     	XXXX:XX:XX.2	56e203e9-864f-49a7-b94b-12284c31e02b (hem-lpbk)
    # Copy Engine      	XXXX:XX:XX.4	44bfc10d-b42a-44e5-bd42-57dc93ea7f91
    # HE-MEM        	XXXX:XX:XX.5	8568ab4e-6ba5-4616-bb65-2a578330a8eb (hem-mem)
    # HE-HSSI       	XXXX:XX:XX.6	823c334c-98bf-11ea-bb37-0242ac130002
    # MEM-TG        	XXXX:XX:XX.7	4dadea34-2c78-48cb-a3dc-5b831f5cecbb (mem_tg)
    # Actually, the current Hitek FIM has:
    # VirtIO Stub      	XXXX:XX:XX.4	3e7b60a0-df2d-4850-aa31-f54a3e403501
    # VirtIO Stub      	XXXX:XX:XX.6	3e7b60a0-df2d-4850-aa31-f54a3e403501

    sudo opae.io init -d ${PAC_PCIE_SBD}.1 $USER:$USER
    sudo opae.io init -d ${PAC_PCIE_SBD}.2 $USER:$USER
    sudo opae.io init -d ${PAC_PCIE_SBD}.3 $USER:$USER
    sudo opae.io init -d ${PAC_PCIE_SBD}.4 $USER:$USER
    sudo opae.io init -d ${PAC_PCIE_SBD}.5 $USER:$USER # AFU 
    sudo opae.io init -d ${PAC_PCIE_SBD}.6 $USER:$USER # AFU 
    sudo opae.io init -d ${PAC_PCIE_SBD}.7 $USER:$USER # AFU 

    SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
    echo "[INFO] Setup $SRIOV_NUMVF sriov_numvfs"
    fpgainfo port > fpgainfo_port.new.log
fi

# Export VF numbers
export PCIE_HEM_MEM=${PAC_PCIE_SBD}.5
export PCIE_HEM_LPBK=${PAC_PCIE_SBD}.2
# export PCIE_HEM_LPBK_bis=${PAC_PCIE_SBD}.6
export PCIE_HEM_MEM_TG=${PAC_PCIE_SBD}.7

sudo chown $USER /etc/opae/