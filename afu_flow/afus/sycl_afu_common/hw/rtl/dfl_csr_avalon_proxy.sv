// CSR wrapper for DFL
// Based on examples-afu/tutorial/afu_types/01_pim_ifc/hello_world/hw/rtl/avalon/hello_world_avalon.sv

`include "ofs_plat_if.vh"
`include "afu_json_info.vh"

import ofs_plat_host_chan_pkg::*;

module dfl_csr_avalon_proxy #(
    parameter logic [ofs_plat_host_chan_pkg::MMIO_ADDR_WIDTH_BYTES-1 : 0] REGISTER_MAP_OFFSET = 'h40
    ) (
    input  logic                     clock_i, 
    input  logic                     reset_ni,
    ofs_plat_avalon_mem_if.to_source  csr_mmio64_to_afu,     // to ofs_plat_afu      
    ofs_plat_avalon_mem_if.to_sink  csr_mmio64_to_kernel   // to kernel
    );

    // =========================================================================
    //
    //   Kernel / DFL muxing based on the incoming address.
    //
    // =========================================================================

    // Internal buffer interface
    ofs_plat_avalon_mem_if # (
      `HOST_CHAN_AVALON_MMIO_PARAMS(64),
      .LOG_CLASS(ofs_plat_log_pkg::HOST_CHAN)
      )
    csr_mmio64_local();

    // Does the address' request fall in the DFL address range
    logic is_dfl_kernel_n;

    // Compose address mask    
    localparam DFL_ADDR_MASK_ZEROS = $clog2(REGISTER_MAP_OFFSET);
    localparam DFL_ADDR_MASK_ONES = ofs_plat_host_chan_pkg::ADDR_WIDTH_LINES - DFL_ADDR_MASK_ZEROS;
    logic [ofs_plat_host_chan_pkg::ADDR_WIDTH_LINES -1 : 0] DFL_ADDR_MASK;
    assign DFL_ADDR_MASK = {{(DFL_ADDR_MASK_ONES){1'b1}}, {(DFL_ADDR_MASK_ZEROS){1'b0}}};
    // The kernel needs to loose 3 bits since words are 8-bytes long
    assign is_dfl_kernel_n = ( {csr_mmio64_to_afu.address, 3'b000} & DFL_ADDR_MASK ) == '0;

    // Pass through and mux the interface, except for the address field
    // We can't just use ofs_plat_avalon_mem_rdwr_if_connect here    
    always_comb begin : kernel_interface

        // Pass-through clock and reset
        // csr_mmio64_to_kernel.clk     = clock_i;
        csr_mmio64_to_kernel.reset_n = reset_ni;
        csr_mmio64_local.reset_n = reset_ni;
        
        // Tie-off unimplemented signals
        // TODO: should we implement these...?
        csr_mmio64_to_kernel.readresponseuser   = '0;
        csr_mmio64_to_kernel.writeresponsevalid = '0;
        csr_mmio64_to_kernel.writeresponse      = '0;
        csr_mmio64_to_kernel.writeresponseuser  = '0;

        // Input
        // We need to:
        //  - extend the address of 3 LSBs, since the module kernel_system is going to ingnore the 3 LSBs (because it addresses 64-bits words)
        //  - subtract the offset of the kernel address space
        //  - zero-extend to full width
        csr_mmio64_to_kernel.address = {csr_mmio64_to_afu.address, 3'b000} - REGISTER_MAP_OFFSET;
        // Disable incoming requests towards the kernel
        csr_mmio64_to_kernel.write           = ( is_dfl_kernel_n ) ? 1'b0 : csr_mmio64_to_afu.write;
        csr_mmio64_to_kernel.read            = ( is_dfl_kernel_n ) ? 1'b0 : csr_mmio64_to_afu.read;
        // Just pass through the rest of the signals
        csr_mmio64_to_kernel.burstcount      = csr_mmio64_to_afu.burstcount;
        csr_mmio64_to_kernel.writedata       = csr_mmio64_to_afu.writedata;
        csr_mmio64_to_kernel.byteenable      = csr_mmio64_to_afu.byteenable;
        csr_mmio64_to_kernel.user            = csr_mmio64_to_afu.user;

        // Also forward the incoming requests to the local address space
        csr_mmio64_local.address             = csr_mmio64_to_afu.address;
        csr_mmio64_local.write               = csr_mmio64_to_afu.write;
        csr_mmio64_local.read                = csr_mmio64_to_afu.read;
        csr_mmio64_local.burstcount          = csr_mmio64_to_afu.burstcount;
        csr_mmio64_local.writedata           = csr_mmio64_to_afu.writedata;
        csr_mmio64_local.byteenable          = csr_mmio64_to_afu.byteenable;
        csr_mmio64_local.user                = csr_mmio64_to_afu.user;

        // Output
        // Multiplex between the local responses (csr_mmio64_local) and the kernel ones (csr_mmio64_to_kernel)
        csr_mmio64_to_afu.waitrequest        = ( is_dfl_kernel_n ) ? csr_mmio64_local.waitrequest        : csr_mmio64_to_kernel.waitrequest;
        csr_mmio64_to_afu.readdatavalid      = ( is_dfl_kernel_n ) ? csr_mmio64_local.readdatavalid      : csr_mmio64_to_kernel.readdatavalid;
        csr_mmio64_to_afu.readdata           = ( is_dfl_kernel_n ) ? csr_mmio64_local.readdata           : csr_mmio64_to_kernel.readdata;
        csr_mmio64_to_afu.response           = ( is_dfl_kernel_n ) ? csr_mmio64_local.response           : csr_mmio64_to_kernel.response;
        csr_mmio64_to_afu.readresponseuser   = ( is_dfl_kernel_n ) ? csr_mmio64_local.readresponseuser   : csr_mmio64_to_kernel.readresponseuser;
        csr_mmio64_to_afu.writeresponsevalid = ( is_dfl_kernel_n ) ? csr_mmio64_local.writeresponsevalid : csr_mmio64_to_kernel.writeresponsevalid;
        csr_mmio64_to_afu.writeresponse      = ( is_dfl_kernel_n ) ? csr_mmio64_local.writeresponse      : csr_mmio64_to_kernel.writeresponse;
        csr_mmio64_to_afu.writeresponseuser  = ( is_dfl_kernel_n ) ? csr_mmio64_local.writeresponseuser  : csr_mmio64_to_kernel.writeresponseuser;

    end : kernel_interface
    
    // =========================================================================
    //
    //   CSR (MMIO) handling with Avalon.
    //
    // =========================================================================

    //
    // The Avalon interface is defined in
    // $OPAE_PLATFORM_ROOT/hw/lib/build/platform/ofs_plat_if/rtl/base_ifcs/avalon/ofs_plat_avalon_mem_if.sv.
    //

    // The AFU ID is a unique ID for a given program.  Here we generated
    // one with the "uuidgen" program and stored it in the AFU's JSON file.
    // ASE and synthesis setup scripts automatically invoke afu_json_mgr
    // to extract the UUID into afu_json_info.vh.
    logic [127:0] afu_id = `AFU_ACCEL_UUID;

    //
    // A valid AFU must implement a device feature list, starting at MMIO
    // address 0.  Every entry in the feature list begins with 5 64-bit
    // words: a device feature header, two AFU UUID words and two reserved
    // words.
    //

    // Is a CSR read request active this cycle?
    logic is_csr_read;
    assign is_csr_read = csr_mmio64_local.read & is_dfl_kernel_n;

    // Is a CSR write request active this cycle?
    logic is_csr_write;
    assign is_csr_write = csr_mmio64_local.write & is_dfl_kernel_n;

    //
    // Receive MMIO read requests
    //

    // Always ready for a new CSR request
    assign csr_mmio64_local.waitrequest = 1'b0;

    //
    // Implement the device feature list by responding to MMIO reads.
    //
    always_ff @(posedge clock_i) begin : mmio_read
        // New read response? Avalon responses have no flow control.
        csr_mmio64_local.readdatavalid <= is_csr_read;

        csr_mmio64_local.response <= '0;
        csr_mmio64_local.readresponseuser <= csr_mmio64_local.user;

        // Avalon addresses are in the space of the data bus width.
        case ( csr_mmio64_local.address[2:0] ) // rd_eff_address[2:0] 
          0: // AFU DFH (device feature header)
            begin
                // Here we define a trivial feature list.  In this
                // example, our AFU is the only entry in this list.
                csr_mmio64_local.readdata <= '0;
                // Feature type is AFU
                csr_mmio64_local.readdata[63:60] <= 4'h1;
                // End of list (last entry in list)
                csr_mmio64_local.readdata[40] <= 1'b1;
            end

          // AFU_ID_L
          1: csr_mmio64_local.readdata <= afu_id[63:0];

          // AFU_ID_H
          2: csr_mmio64_local.readdata <= afu_id[127:64];

          // DFH_RSVD0
          3: csr_mmio64_local.readdata <= '0;

          // DFH_RSVD1
          4: csr_mmio64_local.readdata <= '0;

          default: csr_mmio64_local.readdata <= '0;
        endcase // rd_eff_address[2:0] 

        if (!reset_ni) begin
            csr_mmio64_local.readdatavalid <= 1'b0;
        end
    end : mmio_read

    // Write response
    // This address space is write-ignored
    always_ff @(posedge clock_i) begin : mmio_write_resp
        csr_mmio64_local.writeresponsevalid <= is_csr_write;
        csr_mmio64_local.writeresponse <= '0;
        csr_mmio64_local.writeresponseuser <= csr_mmio64_local.user;

        if (!reset_ni) begin
            csr_mmio64_local.writeresponsevalid <= 1'b0;
        end
    end : mmio_write_resp

endmodule : dfl_csr_avalon_proxy