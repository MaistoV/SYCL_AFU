// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT

`include "ofs_plat_if.vh"

//
// Top level PIM-based module.
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
      .to_fiu          ( plat_ifc.host_chan.ports[0] ),
      .host_mem_to_afu ( host_mem                    ),
      .mmio_to_afu     ( csr_mmio64_to_afu           ),

      // No clock crossing
      .afu_clk         ( ),
      .afu_reset_n     ( )
    );


    // Each interface names its associated clock and reset.
    logic clk;
    assign clk = host_mem.clk;
    logic reset_n;
    assign reset_n = host_mem.reset_n;


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
    //   Instantiate the SCYL IP
    //
    // =========================================================================
    logic kernel_system_mem2_w_writeack;
    // always_ff @(posedge clk) begin
    //     kernel_system_mem2_w_writeack <= kernel_mem[m].writeack;
    //     if (!reset_n) kernel_system_mem2_w_writeack <= 'b0;
    // end

    // TODO: intercept and trigget AVMM writes with ofs_plat_host_chan_avalon_mem_pkg::HC_AVALON_UFLAG_INTERRUPT
    // see bsp_host_mem_if_mux

    // NOTE: burstcount interfaces are not going to match with submodule rs_sycl_ip_RS_3_2_report_di
    kernel_system kernel_system_inst (
      .clock_reset_clk           ( clk                               ),  // input logic 
      .clock_reset_reset_reset_n ( reset_n                           ),  // input logic 
      .cc_snoop_clk_clk          ( /* Unused */                      ),  // input logic 
      // AVM mem1_r
      .mem1_r_enable             ( /* TBD: tie to 1'b1? */           ),  // output logic 
      .mem1_r_read               ( host_mem.rd_read                  ),  // output logic 
      .mem1_r_address            ( host_mem.rd_address               ),  // output logic [40:0] 
      .mem1_r_byteenable         ( host_mem.rd_byteenable            ),  // output logic [63:0]
      .mem1_r_waitrequest        ( host_mem.rd_waitrequest           ),  // input logic 
      .mem1_r_readdata           ( host_mem.rd_readdata              ),  // input logic [511:0]
      .mem1_r_readdatavalid      ( host_mem.rd_readdatavalid         ),  // input logic 
      .mem1_r_burstcount         ( host_mem.rd_burstcount            ),  // output logic 
      // AVM mem2_w
      .mem2_w_enable             ( /* TBD: tie to 1'b1? */           ),   // output logic 
      .mem2_w_write              ( host_mem.wr_write                 ),  // output logic 
      .mem2_w_address            ( host_mem.wr_address               ),  // output logic [40:0] 
      .mem2_w_writedata          ( host_mem.wr_writedata             ),  // output logic [511:0]
      .mem2_w_byteenable         ( host_mem.wr_byteenable            ),  // output logic [63:0]
      .mem2_w_waitrequest        ( host_mem.wr_waitrequest           ),  // input logic 
      .mem2_w_burstcount         ( host_mem.wr_burstcount            ),  // output logic 
      .mem2_w_writeack           ( kernel_system_mem2_w_writeack     ),  // input logic 
      // AVS kernel_cra
      .kernel_cra_debugaccess    ( /* TBD */                         ),  // input logic 
      .kernel_cra_burstcount     ( csr_mmio64_to_afu.burstcount      ),  // input logic 
      .kernel_cra_enable         ( /* TBD: tie to 1'b1? */           ),   // input logic 
      .kernel_cra_read           ( csr_mmio64_to_afu.read            ),  // input logic 
      .kernel_cra_write          ( csr_mmio64_to_afu.write           ),  // input logic 
      .kernel_cra_address        ( csr_mmio64_to_afu.address         ),  // input logic [29:0] 
      .kernel_cra_writedata      ( csr_mmio64_to_afu.writedata       ),  // input logic [511:0]
      .kernel_cra_byteenable     ( csr_mmio64_to_afu.byteenable      ),  // input logic [63:0]
      .kernel_cra_waitrequest    ( csr_mmio64_to_afu.waitrequest     ),  // output logic 
      .kernel_cra_readdata       ( csr_mmio64_to_afu.readdata        ),  // output logic [511:0]
      .kernel_cra_readdatavalid  ( csr_mmio64_to_afu.readdatavalid   ),  // output logic 
      .kernel_irq_irq            ( /* TBD: intercept and inject on AVMM bus */ ),   // output logic 
      .device_exception_bus      ( /* TBD: keep open? */             )   // output logic [511:0]
    );

  // rs_sycl_ip_RS_3_2_report_di rs_sycl_ip_RS_3_2_report_di_inst (
  //   // Interface: clock (clock end)
  //   .clock                                     ( clk                              ), // 1-bit clk input
  //   // Interface: resetn (reset end)
  //   .resetn                                    ( reset_n                          ), // 1-bit reset_n input
  //   // Interface: device_exception_bus (conduit end)
  //   .device_exception_bus                      ( /* TBD */ ), // 64-bit data output
  //   // Interface: kernel_irqs (interrupt end)
  //   .kernel_irqs                               ( /* TBD */ ), // 1-bit irq output
  //   // Interface: avm_mem_gmem_0_1_port_0_0_rw (avalon start)
  //   // .avm_mem_gmem_0_1_port_0_0_rw_enable       ( /* TBD */ ),
  //   .avm_mem_gmem_0_1_port_0_0_rw_address      ( host_mem.rd_address              ), // 41-bit address output
  //   .avm_mem_gmem_0_1_port_0_0_rw_byteenable   ( host_mem.rd_byteenable           ), // 8-bit byteenable output
  //   .avm_mem_gmem_0_1_port_0_0_rw_readdatavalid( host_mem.rd_readdatavalid        ), // 1-bit readdatavalid input
  //   .avm_mem_gmem_0_1_port_0_0_rw_read         ( host_mem.rd_read                 ), // 1-bit read output
  //   .avm_mem_gmem_0_1_port_0_0_rw_readdata     ( host_mem.rd_readdata             ), // 64-bit readdata input
  //   .avm_mem_gmem_0_1_port_0_0_rw_waitrequest  ( host_mem.rd_waitrequest          ), // 1-bit waitrequest input
  //   .avm_mem_gmem_0_1_port_0_0_rw_burstcount   ( host_mem.rd_burstcount           ), // 1-bit burstcount output
  //   // Interface: avm_mem_gmem_1_2_port_0_0_rw (avalon start)
  //   .avm_mem_gmem_1_2_port_0_0_rw_address      ( host_mem.wr_address              ), // 41-bit address output
  //   .avm_mem_gmem_1_2_port_0_0_rw_byteenable   ( host_mem.wr_byteenable           ), // 8-bit byteenable output
  //   .avm_mem_gmem_1_2_port_0_0_rw_write        ( host_mem.wr_write                ), // 1-bit write output
  //   .avm_mem_gmem_1_2_port_0_0_rw_writedata    ( host_mem.wr_writedata            ), // 64-bit writedata output
  //   .avm_mem_gmem_1_2_port_0_0_rw_waitrequest  ( host_mem.wr_waitrequest          ), // 1-bit waitrequest input
  //   .avm_mem_gmem_1_2_port_0_0_rw_burstcount   ( host_mem.wr_burstcount           ), // 1-bit burstcount output
  //   // Interface: csr_ring_root_avs (avalon end)
  //   .csr_ring_root_avs_read                    ( csr_mmio64_to_afu.read           ), // 1-bit read input
  //   .csr_ring_root_avs_readdata                ( csr_mmio64_to_afu.readdata       ), // 64-bit readdata output
  //   .csr_ring_root_avs_readdatavalid           ( csr_mmio64_to_afu.readdatavalid  ), // 1-bit readdatavalid output
  //   .csr_ring_root_avs_write                   ( csr_mmio64_to_afu.write          ), // 1-bit write input
  //   .csr_ring_root_avs_writedata               ( csr_mmio64_to_afu.writedata      ), // 64-bit writedata input
  //   .csr_ring_root_avs_address                 ( csr_mmio64_to_afu.address        ), // 5-bit address input
  //   .csr_ring_root_avs_byteenable              ( csr_mmio64_to_afu.byteenable     ), // 8-bit byteenable input
  //   .csr_ring_root_avs_waitrequest             ( csr_mmio64_to_afu.waitrequest    )  // 1-bit waitrequest output
  // );


endmodule
