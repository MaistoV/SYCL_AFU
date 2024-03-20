// Injects IRQs on the AVMM host_mem interface 
//     - intercept and trigger AVMM writes with ofs_plat_host_chan_avalon_mem_pkg::HC_AVALON_UFLAG_INTERRUPT
// Based on <asp-full-path>/bsp_host_mem_if_mux

`include "ofs_plat_if.vh"

module avalon_interrupt_proxy (
    input  logic                             clock_i,
    input  logic                             reset_ni,
    input  logic                             kernel_irq_i,      // From kernel_system
    ofs_plat_avalon_mem_rdwr_if.to_source    host_mem_kernel,   // From kernel_system
    ofs_plat_avalon_mem_rdwr_if.to_sink      host_mem_plat      // To ofs_plat_afu
    );
    
    // dummy pass-throguh for now
    ofs_plat_avalon_mem_rdwr_if_connect pass_through_interfaces_inst (
        .mem_sink   ( host_mem_plat   ),
        .mem_source ( host_mem_kernel )
    );

endmodule : avalon_interrupt_proxy