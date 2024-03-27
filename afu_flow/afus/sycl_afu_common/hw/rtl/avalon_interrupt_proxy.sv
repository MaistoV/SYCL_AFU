// Injects IRQs on the AVMM host_mem interface 
//     - intercept and trigger AVMM writes with ofs_plat_host_chan_avalon_mem_pkg::HC_AVALON_UFLAG_INTERRUPT
// Based on <asp-full-path>/bsp_host_mem_if_mux

`include "ofs_plat_if.vh"

module avalon_interrupt_proxy #(
    parameter logic DISABLE = 1'b0    // Disable interrupt injection
    ) (
    input  logic                             clock_i,
    input  logic                             reset_ni,
    input  logic                             enable_i,
    input  logic                             kernel_irq_i,      // From kernel_system
    ofs_plat_avalon_mem_rdwr_if.to_source    host_mem_kernel,   // From kernel_system
    ofs_plat_avalon_mem_rdwr_if.to_sink      host_mem_plat      // To ofs_plat_afu
    );
    
    if ( DISABLE ) begin : no_inject
        // dummy pass-through
        ofs_plat_avalon_mem_rdwr_if_connect pass_through_interfaces_inst (
            .mem_sink   ( host_mem_plat   ),
            .mem_source ( host_mem_kernel )
        );
    end : no_inject
    else begin : inject
        logic gated_irq;
        assign gated_irq = kernel_irq_i & enable_i;

        bsp_host_mem_if_mux bsp_host_mem_if_mux_inst (
            .clk           ( clock_i         ),
            .reset         ( ~reset_ni       ),
            .bsp_irq       ( gated_irq       ),
            .host_mem_if   ( host_mem_plat   ), // to_sink
            .bsp_mem_if    ( host_mem_kernel ), // to_source
            .wr_fence_flag ( 1'b0            )  // Tie to zero
        );

        always_comb begin : throw_warning
            if ( kernel_irq_i ) begin
                $warning ("Injecting interrupts assuming no wr_fence_flag");
            end
        end : throw_warning
    end : inject
    
endmodule : avalon_interrupt_proxy