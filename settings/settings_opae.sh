#!/binbash

# Initial setup
SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
echo "Found $SRIOV_NUMVF sriov_numvfs"
if [ $SRIOV_NUMVF == "0" ]; then
    PCIE_BDF=$(lspci | grep bcce | head -n1 | awk '{print $1}')
    # Create 3 VFs ( XXXX:XX:XX.5-7)
    sudo pci_device  0000:$PCIE_BDF vf 3

    # Bind VFs to user
    # NOTE: VF enumeration is be deterministic
    # From intel-ofs-docs-main/n6000/user_guides/ofs_getting_started/ug_qs_ofs_n6000.html
    #   Component 	    VF 	            Accelerator GUID
    #   base PF 	    XXXX:XX:XX.0 	N/A
    #   VirtIO Stub 	XXXX:XX:XX.1 	3e7b60a0-df2d-4850-aa31-f54a3e403501
    #   HE-MEM Stub 	XXXX:XX:XX.2 	56e203e9-864f-49a7-b94b-12284c31e02b
    #   Copy Engine 	XXXX:XX:XX.4 	44bfc10d-b42a-44e5-bd42-57dc93ea7f91
    #   HE-MEM 	        XXXX:XX:XX.5 	8568ab4e-6ba5-4616-bb65-2a578330a8eb
    #   HE-HSSI 	    XXXX:XX:XX.6 	823c334c-98bf-11ea-bb37-0242ac130002
    #   MEM-TG 	        XXXX:XX:XX.7 	4dadea34-2c78-48cb-a3dc-5b831f5cecbb
    sudo opae.io init -d 0000:01:00.1 $USER:$USER
    sudo opae.io init -d 0000:01:00.2 $USER:$USER
    sudo opae.io init -d 0000:01:00.4 $USER:$USER
    sudo opae.io init -d 0000:01:00.5 $USER:$USER
    sudo opae.io init -d 0000:01:00.6 $USER:$USER
    sudo opae.io init -d 0000:01:00.7 $USER:$USER
    
    SRIOV_NUMVF=$(sudo find /sys/ -name "sriov_numvfs" | head -n1 | xargs cat)
    echo "Setup $SRIOV_NUMVF sriov_numvfs"
    fpgainfo port | grep -E "PCIe|GUID"
fi