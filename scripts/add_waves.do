# TODO: extend for new flow

#add wave -position insertpoint vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/*
add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/afu_clk
add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/reset
add wave                           vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/hls_irq
add wave -group master_read	    -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/requestor_avmm_rd_*
add wave -group master_write    -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/requestor_avmm_wr_*	
add wave -group MMIO            -r vsim:/ase_top/ase_top_generic/platform_shim_ccip_std_afu/ccip_std_afu/afu_inst/mmio_avmm_*	