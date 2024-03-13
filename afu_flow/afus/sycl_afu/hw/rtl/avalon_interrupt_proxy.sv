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
    // always_comb begin : tie_interfaces
    //     // Read bus
    //     host_mem_plat.rd_waitrequest = host_mem_plat.rd_waitrequest;
    //     host_mem_plat.rd_readdata = host_mem_plat.rd_readdata;
    //     host_mem_plat.rd_readdatavalid = host_mem_plat.rd_readdatavalid;
    //     host_mem_plat.rd_response = host_mem_plat.rd_response;
    //     host_mem_plat.rd_readresponseuser = host_mem_plat.rd_readresponseuser;

    //     host_mem_plat.rd_address = host_mem_kernel.rd_address;
    //     host_mem_plat.rd_read = host_mem_kernel.rd_read;
    //     host_mem_plat.rd_burstcount = host_mem_kernel.rd_burstcount;
    //     host_mem_plat.rd_byteenable = host_mem_kernel.rd_byteenable;
    //     host_mem_plat.rd_user = host_mem_kernel.rd_user;

    //     // Write bus
    //     host_mem_plat.wr_waitrequest = host_mem_plat.wr_waitrequest;
    //     host_mem_plat.wr_writeresponsevalid = host_mem_plat.wr_writeresponsevalid;
    //     host_mem_plat.wr_response = host_mem_plat.wr_response;
    //     host_mem_plat.wr_writeresponseuser = host_mem_plat.wr_writeresponseuser;

    //     host_mem_plat.wr_address = host_mem_kernel.wr_address;
    //     host_mem_plat.wr_write = host_mem_kernel.wr_write;
    //     host_mem_plat.wr_burstcount = host_mem_kernel.wr_burstcount;
    //     host_mem_plat.wr_writedata = host_mem_kernel.wr_writedata;
    //     host_mem_plat.wr_byteenable = host_mem_kernel.wr_byteenable;
    //     host_mem_plat.wr_user = host_mem_kernel.wr_user;
    // end : tie_interfaces

    ofs_plat_avalon_mem_rdwr_if_connect tie_interfaces_inst (
        .mem_sink   ( host_mem_plat   ),
        .mem_source ( host_mem_kernel )
    );

endmodule : avalon_interrupt_proxy