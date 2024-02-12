# TODO: extend for new flow

# Only show leaf names
config wave -signalnamewidth 1

add wave -group AFU  vsim:/ase_top/ase_top_plat/ase_afu_main_pcie_ss/ase_afu_main_emul/afu_main/port_afu_instances/ofs_plat_afu/afu/*

# OLD OPAE
# add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/afu_clk
# add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/reset
# add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/hls_irq
# add wave -group master_read	    -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/requestor_avmm_rd_*
# add wave -group master_write    -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/requestor_avmm_wr_*	
# add wave -group MMIO            -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/mmio_avmm_*	