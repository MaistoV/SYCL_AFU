// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

`include "ofs_plat_if.vh"

//
// Top level PIM-based module with SYCL kernel wrapper array.
//

module ofs_plat_afu (
    // All platform wires, wrapped in one interface.
    ofs_plat_if plat_ifc
    );

    ////////////////////////////
    //  Tie off unused ports. //
    ////////////////////////////
    ofs_plat_if_tie_off_unused # (
      .HOST_CHAN_IN_USE_MASK ( -1 ), // Use all host memory channels
      .LOCAL_MEM_IN_USE_MASK (  0 ),
      .HSSI_IN_USE_MASK      (  0 ),
      .OTHER_IN_USE_MASK     (  0 ) 
    )
    tie_off (
        plat_ifc
    );

    ///////////////////////////////////////
    // Generate AFU array and interfaces //
    ///////////////////////////////////////
    // Ports of offset 8 are not going to bind to OPAE-vfio
    // VF mapping, wlog S:B:D=XX:XX:00
    // - 00.0 -> DFL
    // - 00.1..NUM_SR_PORTS (FIM AFUs) -> VFIO 
    // - 00.x -> VFIO 
    // - 00.7 -> VFIO
    // - 01.0 -> DFL again
    // - 01.1 -> VFIO
    // - ... 
    // - 01.7 -> VFIO
    // - 02.0 -> DFL again
    // NOTE: assume only 
    localparam int NULL_PORT = 8 - top_cfg_pkg::NUM_SR_PORTS;
    // Assertion
    assert property ( top_cfg_pkg::NUM_SR_PORTS < 8 )
        else $fatal(1, "NULL_PORT (%d) must be less than 8", NULL_PORT);
    assert property ( ( top_cfg_pkg::PG_NUM_PORT +  top_cfg_pkg::NUM_SR_PORTS) < 16 ) 
        else $fatal(1, "Unsupported (for now) PG_NUM_PORT (%d) + NUM_SR_PORTS (%d) > 16", NULL_PORT);

    generate
      for ( genvar port = 0; port < `OFS_PLAT_PARAM_HOST_CHAN_NUM_PORTS; port++ ) begin : afu_array

        // Tie-off buggy port
        if ( port == NULL_PORT ) begin : tie_off
          // Instatiate a tie-off module
          ofs_plat_host_chan_fiu_if_tie_off null_tie_off (
            .port ( plat_ifc.host_chan.ports[port] )
          );
        end : tie_off
        else begin : gen_afu
          
          ////////////////
          // Interfaces // 
          ////////////////

          // Host memory interface
          ofs_plat_avalon_mem_rdwr_if # (
            `HOST_CHAN_AVALON_MEM_RDWR_PARAMS,
            .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
          )
          host_mem();

          // CSR interface
          ofs_plat_avalon_mem_if # (
            `HOST_CHAN_AVALON_MMIO_PARAMS(64),
            .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
            )
          csr_mmio64_to_afu();

          ///////////////////////////////////////
          // Clocks and resets into interfaces //
          ///////////////////////////////////////
          
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
            .to_fiu          ( plat_ifc.host_chan.ports[port] ),
            .host_mem_to_afu ( host_mem                    ), // to_source_clk
            .mmio_to_afu     ( csr_mmio64_to_afu           ), // to_sink_clk

            // No clock crossing
            .afu_clk         ( ),
            .afu_reset_n     ( )
          );


          /////////////////////////////////////
          // Instantiate the SCYL IP wrapper //
          /////////////////////////////////////
          
          kernel_dfl_wrapper # (
          `ifdef DISABLE_AVMM_INTERRUPT
            .DISABLE_AVMM_INTERRUPT     ( `DISABLE_AVMM_INTERRUPT         ),
          `endif // DISABLE_AVMM_INTERRUPT
          `ifdef KERNEL_REGISTER_MAP_OFFSET_HEX
            .KERNEL_REGISTER_MAP_OFFSET ( `KERNEL_REGISTER_MAP_OFFSET_HEX )
          `endif // KERNEL_REGISTER_MAP_OFFSET_HEX
          ) kernel_dfl_wrapper_inst (
            .clock_i           ( clk               ), 
            .reset_ni          ( reset_n           ),
            .host_mem_plat     ( host_mem          ), // to_sink
            .csr_mmio64_to_afu ( csr_mmio64_to_afu )  // to_source
          );
        end : gen_afu
      end : afu_array
    endgenerate
endmodule
