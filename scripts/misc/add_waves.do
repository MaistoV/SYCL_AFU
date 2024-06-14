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

# AVMM bridges
catch {
    add wave -group kernel_dfl_wrapper -group avmm_bridges  -group csr_mmio64_to_afu_bridge  vsim:$ROOT_KERNEL_DFL_WRAPPER/csr_mmio64_to_afu_bridge/*
    add wave -group kernel_dfl_wrapper -group avmm_bridges  -group host_mem_plat_bridge      vsim:$ROOT_KERNEL_DFL_WRAPPER/host_mem_plat_bridge/*
    add wave -group kernel_dfl_wrapper -group avmm_bridges  -group kernel_rd_bridge_inst     vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_rd_bridge_inst/*
    add wave -group kernel_dfl_wrapper -group avmm_bridges  -group kernel_wr_bridge_inst     vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_rd_bridge_inst/*
    add wave -group kernel_dfl_wrapper -group avmm_bridges  -group kernel_cra_bridge_inst    vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_cra_bridge_inst/*
}

# AVMM splitter
add wave -group kernel_dfl_wrapper -group avmm_splitter -group master                    vsim:$ROOT_KERNEL_DFL_WRAPPER/avmm_splitter_qsys_inst/avmm_host_0*
add wave -group kernel_dfl_wrapper -group avmm_splitter -group slave_csr                 vsim:$ROOT_KERNEL_DFL_WRAPPER/avmm_splitter_qsys_inst/avmm_agent_0*
add wave -group kernel_dfl_wrapper -group avmm_splitter -group slave_kernel              vsim:$ROOT_KERNEL_DFL_WRAPPER/avmm_splitter_qsys_inst/avmm_agent_1*

# Kernel system
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/clock_reset_clk
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/clock_reset_reset_reset_n
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/cc_snoop_clk_clk
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/kernel_irq_irq
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/device_exception_bus
add wave -group kernel_dfl_wrapper -group kernel_system                                  vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/sys_cra_ring_address
# SYCL IP
add wave -group kernel_dfl_wrapper -group kernel_system -group SYCL_IP                   vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/$::env(SYCL_IP_NAME)_report_sys/*
add wave -group kernel_dfl_wrapper -group kernel_system -group mem1_r                    vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/mem1_r_*
add wave -group kernel_dfl_wrapper -group kernel_system -group mem2_w                    vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/mem2_w_*
add wave -group kernel_dfl_wrapper -group kernel_system -group cra                       vsim:$ROOT_KERNEL_DFL_WRAPPER/kernel_system_inst/kernel_cra_*

# DFL CSR 
add wave -group kernel_dfl_wrapper -group DFL_CSR                                   vsim:$ROOT_KERNEL_DFL_WRAPPER/dfl_csr_avalon_inst/*

# AVMM interrupt proxy
add wave -group kernel_dfl_wrapper -group AVMM_intr                                     vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/*
# Instantiating bsp_host_mem_if_mux_inst is optional
catch {
    add wave -group kernel_dfl_wrapper -group AVMM_intr     -group bsp_host_mem_if_mux      vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/bsp_host_mem_if_mux_inst/*
}
add wave -group kernel_dfl_wrapper -group AVMM_intr     -group host_mem_plat            vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/host_mem_plat/*
add wave -group kernel_dfl_wrapper -group AVMM_intr     -group host_mem_kernel          vsim:$ROOT_KERNEL_DFL_WRAPPER/avalon_interrupt_proxy_inst/host_mem_kernel/*
