// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

`include "ofs_plat_if.vh"

//
// Top level PIM-based module.
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
          ofs_plat_host_chan_fiu_if_tie_off null_tie_off (
            .port ( plat_ifc.host_chan.ports[port] )
          );
        end : tie_off
        else begin : gen_afu

          ////////////////
          // Interfaces // 
          ////////////////

          // Host memory interface
          ofs_plat_axi_mem_if # (
            // The PIM provides parameters for configuring a standard host
            // memory DMA AXI memory interface.
            `HOST_CHAN_AXI_MEM_PARAMS,
            // PIM interfaces can be configured to log traffic during
            // simulation. In ASE, see work/log_ofs_plat_host_chan.tsv.
            .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
          )
          host_mem();

          // CSR interface
          ofs_plat_axi_mem_lite_if # (
            `HOST_CHAN_AXI_MMIO_PARAMS(64),
            .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
            )
          csr_mmio64_to_afu();
          
          ///////////
          // Shims //
          ///////////

          ofs_plat_host_chan_as_axi_mem_with_mmio # (
            .ADD_CLOCK_CROSSING     ( 0 ),
            .ADD_TIMING_REG_STAGES  ( 0 ),
            .SORT_READ_RESPONSES    ( 1 ),
            .SORT_WRITE_RESPONSES   ( 0 ),
            .BUFFER_READ_RESPONSES  ( 0 )
          ) primary_axi (
            .to_fiu          ( plat_ifc.host_chan.ports[port] ),
            .host_mem_to_afu ( host_mem                       ),
            .mmio_to_afu     ( csr_mmio64_to_afu              ),

            // No clock crossing
            .afu_clk         ( ),
            .afu_reset_n     ( )
          );

          //////////
          // AFUs //
          //////////

          hello_world_axi # (
            .ID_BYTE ( 'h30 + port ) // Chars '0', '1', and so on
          ) hello_world (
            .mmio64_to_afu ( csr_mmio64_to_afu ),
            .host_mem      ( host_mem          )
          );
        end : gen_afu
      end : afu_array
    endgenerate
endmodule
