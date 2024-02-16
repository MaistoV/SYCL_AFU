# Same user, different VFs, explicit VF
# RESULT: Success
mem_tg         --pci-address $PCIE_HEM_MEM_TG   tg_test &
host_exerciser --pci-address $PCIE_HEM_LPBK     lpbk    &
# host_exerciser --pci-address $PCIE_HEM_LPBK_bis lpbk    &
host_exerciser --pci-address $PCIE_HEM_MEM      mem     &

# Wait for children
wait