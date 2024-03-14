# TODO: extend for new flow

# Only show leaf names
config wave -signalnamewidth 1

# OFS PIM
set ROOT_OFS_PLAT /ase_top/ase_top_plat/ase_afu_main_pcie_ss/ase_afu_main_emul/afu_main/port_afu_instances/ofs_plat_afu
add wave -group OFS_PLAT vsim:$ROOT_OFS_PLAT/*
add wave -group OFS_PLAT -group host_mem vsim:$ROOT_OFS_PLAT/host_mem/*
add wave -group OFS_PLAT -group csr_mmio64_to_afu vsim:$ROOT_OFS_PLAT/csr_mmio64_to_afu/*

# Kernel wrapper
set ROOT_KERNEL_DFL_WRAPPER /ase_top/ase_top_plat/ase_afu_main_pcie_ss/ase_afu_main_emul/afu_main/port_afu_instances/ofs_plat_afu/kernel_dfl_wrapper_inst
add wave -group kernel_dfl_wrapper vsim:$ROOT_KERNEL_DFL_WRAPPER/*
# Kernel system
## RS IP
add wave -group kernel_dfl_wrapper -group kernel_system -group RS_IP    vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/rs_sycl_ip_RS_3_2_report_sys/*
add wave -group kernel_dfl_wrapper -group kernel_system -group mem1_r   vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/mem1_r_*
add wave -group kernel_dfl_wrapper -group kernel_system -group mem2_w   vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/mem2_w_*
add wave -group kernel_dfl_wrapper -group kernel_system -group cra      vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/kernel_cra_*
add wave -group kernel_dfl_wrapper -group kernel_system                 vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/kernel_irq_irq
add wave -group kernel_dfl_wrapper -group kernel_system                 vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/device_exception_bus
# AVMM interrupt proxy
add wave -group kernel_dfl_wrapper -group AVMM_intr     -group host_mem_plat   vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/host_mem_plat/*
add wave -group kernel_dfl_wrapper -group AVMM_intr     -group host_mem_kernel vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/host_mem_kernel/*
# DFL CSR proxy 
add wave -group kernel_dfl_wrapper -group DFL_CSR                                   vsim:$ROOT_KERNEL_DFL_WRAPPER/dfl_csr_avalon_proxy_inst/*
add wave -group kernel_dfl_wrapper -group DFL_CSR       -group csr_mmio64_to_afu    vsim:$ROOT_KERNEL_DFL_WRAPPER/dfl_csr_avalon_proxy_inst/csr_mmio64_to_afu/*
add wave -group kernel_dfl_wrapper -group DFL_CSR       -group csr_mmio64_to_kernel vsim:$ROOT_KERNEL_DFL_WRAPPER/dfl_csr_avalon_proxy_inst/csr_mmio64_to_kernel/*
