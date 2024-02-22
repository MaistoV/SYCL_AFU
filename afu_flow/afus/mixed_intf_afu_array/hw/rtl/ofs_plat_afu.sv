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

    // Assume an even number of ports

    // Generate AXI AFUs
    generate for ( genvar port = 0; port < `OFS_PLAT_PARAM_HOST_CHAN_NUM_PORTS/2; port++ ) begin : afu_array_axi

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

      ////////////
      // Clocks //
      ////////////
      // TODO: is this necessary?
      // logic clk;
      // assign clk = host_mem.clk;
      // logic reset_n;
      // assign reset_n = host_mem.reset_n;
    

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

      hello_world_axi hello_world (
        .mmio64_to_afu ( csr_mmio64_to_afu ),
        .host_mem      ( host_mem          )
      );

    end : afu_array_axi
    endgenerate

    // Generate Avalon AFUs
    generate for ( genvar port = `OFS_PLAT_PARAM_HOST_CHAN_NUM_PORTS/2; port < `OFS_PLAT_PARAM_HOST_CHAN_NUM_PORTS; port++ ) begin : afu_array_avalon

      ////////////////
      // Interfaces // 
      ////////////////

      // Host memory interface
      ofs_plat_avalon_mem_rdwr_if # (
        // The PIM provides parameters for configuring a standard host
        // memory DMA AXI memory interface.
        `HOST_CHAN_AVALON_MEM_RDWR_PARAMS,
        // PIM interfaces can be configured to log traffic during
        // simulation. In ASE, see work/log_ofs_plat_host_chan.tsv.
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
      // TODO: is this necessary?
      // logic clk;
      // assign clk = host_mem.clk;
      // logic reset_n;
      // assign reset_n = host_mem.reset_n;
    

      ///////////
      // Shims //
      ///////////

      ofs_plat_host_chan_as_avalon_mem_rdwr_with_mmio # (
        .ADD_CLOCK_CROSSING     ( 0 ),
        .ADD_TIMING_REG_STAGES  ( 0 )
      ) primary_avalon (
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

      hello_world_avalon hello_world (
        .mmio64_to_afu ( csr_mmio64_to_afu ),
        .host_mem      ( host_mem          )
      );

    end : afu_array_avalon
    endgenerate

endmodule
