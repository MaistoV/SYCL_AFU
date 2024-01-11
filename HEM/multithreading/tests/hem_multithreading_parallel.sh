# Same user, different VFs, explicit VFs
source hem_single_thread.sh host_exerciser "-p $PCIE_HEM_LPBK"       lpbk &
source hem_single_thread.sh host_exerciser "-p $PCIE_HEM_LPBK_bis"   lpbk &
source hem_single_thread.sh host_exerciser "-p $PCIE_HEM_MEM"        mem &
source hem_single_thread.sh mem_tg         "-p $PCIE_HEM_MEM_TG"     tg_test &

# Wait all children
wait