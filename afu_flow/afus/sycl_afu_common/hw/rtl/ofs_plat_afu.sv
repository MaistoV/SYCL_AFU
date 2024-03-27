// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

`include "ofs_plat_if.vh"

//
// Top level PIM-based module with SYCL kernel wrapper.
//

module ofs_plat_afu
   (
    // All platform wires, wrapped in one interface.
    ofs_plat_if plat_ifc
    );

    // ====================================================================
    //
    //  Get an AXI-MM host channel connection from the platform.
    //
    // ====================================================================

    // Host memory interface
    ofs_plat_avalon_mem_rdwr_if # (
      // memory DMA AXI memory interface.
      `HOST_CHAN_AVALON_MEM_RDWR_PARAMS,
      // .BURST_CNT_WIDTH(4), // TODO: tune this
      .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
    )
    host_mem();

    // CSR interface
    ofs_plat_avalon_mem_if # (
      `HOST_CHAN_AVALON_MMIO_PARAMS(64),
      .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
      )
    csr_mmio64_to_afu();

    ////////////
    // Clocks //
    ////////////
    // Each interface names its associated clock and reset.
    logic clk;
    assign clk = host_mem.clk;
    logic reset_n;
    assign reset_n = host_mem.reset_n;

    ///////////
    // Shims //
    ///////////

    ofs_plat_host_chan_as_avalon_mem_rdwr_with_mmio # (
      .ADD_CLOCK_CROSSING     ( 0 ),
      .ADD_TIMING_REG_STAGES  ( 0 )
    ) primary_avalon (
      .to_fiu          ( plat_ifc.host_chan.ports[0] ),
      .host_mem_to_afu ( host_mem                    ), // to_source_clk
      .mmio_to_afu     ( csr_mmio64_to_afu           ), // to_sink_clk

      // No clock crossing
      .afu_clk         ( ),
      .afu_reset_n     ( )
    );


    // ====================================================================
    //
    //  Tie off unused ports.
    //
    // ====================================================================

    // The PIM ties off unused devices, controlled by the AFU indicating
    // which devices it is using. This way, an AFU must know only about
    // the devices it uses. Tie-offs are thus portable, with the PIM
    // managing devices unused by and unknown to the AFU.
    ofs_plat_if_tie_off_unused
      #(
        // Host channel group 0 port 0 is connected. The mask is a
        // bit vector of indices used by the AFU.
        .HOST_CHAN_IN_USE_MASK(1)
        )
        tie_off(plat_ifc);


    // =========================================================================
    //
    //   Instantiate the SCYL IP wrapper
    //
    // =========================================================================
    
    kernel_dfl_wrapper # (
     .DISABLE_AVMM_INTERRUPT     ( `DISABLE_AVMM_INTERRUPT         ),
     .KERNEL_REGISTER_MAP_OFFSET ( `KERNEL_REGISTER_MAP_OFFSET_HEX )
    ) kernel_dfl_wrapper_inst (
      .clock_i           ( clk               ), 
      .reset_ni          ( reset_n           ),
      .host_mem_plat     ( host_mem          ), // to_sink
      .csr_mmio64_to_afu ( csr_mmio64_to_afu )  // to_source
    );


endmodule
