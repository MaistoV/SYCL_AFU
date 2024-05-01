// CSR wrapper for DFL
// Based on examples-afu/tutorial/afu_types/01_pim_ifc/hello_world/hw/rtl/avalon/hello_world_avalon.sv

`include "ofs_plat_if.vh"
`include "afu_json_info.vh"

module dfl_csr_avalon # ( 
    parameter ADDR_WIDTH       = 3,
    parameter DATA_WIDTH       = 64,
    parameter BURST_CNT_WIDTH  = 7,
    parameter RESPONSE_WIDTH   = 2,
    parameter DATA_N_BYTES     = DATA_WIDTH / 8,
    parameter USER_WIDTH       = 8
    ) (
    input  logic                     clock_i, 
    input  logic                     reset_i,
    output logic                     reset_n_kernel_o, 
    output logic                     enable_kernel_irq_o, 

    // AVMM to source
    output logic                              csr_mmio64_to_afu_waitrequest,
    output logic                              csr_mmio64_to_afu_readdatavalid,
    output logic [DATA_WIDTH      -1 : 0]     csr_mmio64_to_afu_readdata,
    output logic [RESPONSE_WIDTH  -1 : 0]     csr_mmio64_to_afu_response,
    // output logic [USER_WIDTH      -1 : 0]     csr_mmio64_to_afu_readresponseuser,
    output logic                              csr_mmio64_to_afu_writeresponsevalid,
    // output logic [RESPONSE_WIDTH  -1 : 0]     csr_mmio64_to_afu_writeresponse,
    // output logic [USER_WIDTH      -1 : 0]     csr_mmio64_to_afu_writeresponseuser,
    input  logic [ADDR_WIDTH      -1 : 0]     csr_mmio64_to_afu_address,
    input  logic                              csr_mmio64_to_afu_write,
    input  logic                              csr_mmio64_to_afu_read,
    input  logic [BURST_CNT_WIDTH -1 : 0]     csr_mmio64_to_afu_burstcount, // Unused
    input  logic [DATA_WIDTH      -1 : 0]     csr_mmio64_to_afu_writedata,
    // input  logic [USER_WIDTH      -1 : 0]     csr_mmio64_to_afu_user,
    input  logic [DATA_N_BYTES    -1 : 0]     csr_mmio64_to_afu_byteenable
    );

    // =========================================================================
    //
    //   CSR address space
    //
    // =========================================================================

    //
    // A valid AFU must implement a device feature list, starting at MMIO
    // address 0.  Every entry in the feature list begins with 5 64-bit
    // words: a device feature header, two AFU UUID words and two reserved
    // words.
    //

    // Device feature list
    localparam AFU_DFH      =  0;
    localparam AFU_ID_L     =  1;
    localparam AFU_ID_H     =  2;
    localparam DFH_RSVD0    =  3;
    localparam DFH_RSVD1    =  4;

    // Non-DFL entries for SYCL kernel control
    localparam AFU_RESET    =  5;   // Reset kernel AFU
    localparam AFU_IRQ_EN   =  6;   // Enable interrupt injectionin AVMM write interface

    // =========================================================================
    //
    //   CSR (MMIO) handling with Avalon.
    //
    // =========================================================================

    // Registers
    logic enable_kernel_irq_d, enable_kernel_irq_q;
    logic reset_n_kernel_d   , reset_n_kernel_q;

    // The AFU ID is a unique ID for a given program.  Here we generated
    // one with the "uuidgen" program and stored it in the AFU's JSON file.
    // ASE and synthesis setup scripts automatically invoke afu_json_mgr
    // to extract the UUID into afu_json_info.vh.
    logic [127:0] afu_id;
    assign afu_id = `AFU_ACCEL_UUID;

    // Is a CSR read request active this cycle?
    logic is_csr_read;
    assign is_csr_read = csr_mmio64_to_afu_read;

    // Is a CSR write request active this cycle?
    logic is_csr_write;
    assign is_csr_write = csr_mmio64_to_afu_write;

    //
    // Receive MMIO read requests
    //

    // Always ready for a new CSR request
    assign csr_mmio64_to_afu_waitrequest = 1'b0;

    //
    // Implement the device feature list by responding to MMIO reads.
    //
    always_ff @(posedge clock_i) begin : mmio_read
        // New read response? Avalon responses have no flow control.
        csr_mmio64_to_afu_readdatavalid <= is_csr_read;

        csr_mmio64_to_afu_response <= '0; // 00: OK
        // csr_mmio64_to_afu_readresponseuser <= csr_mmio64_to_afu_user;

        // Avalon addresses are in the space of the data bus width.
        case ( csr_mmio64_to_afu_address ) // csr_mmio64_to_afu_address
          AFU_DFH   : begin
                // Here we define a trivial feature list.  In this
                // example, our AFU is the only entry in this list.
                csr_mmio64_to_afu_readdata <= '0;
                // Feature type is AFU
                csr_mmio64_to_afu_readdata[63:60] <= 4'h1;
                // End of list (last entry in list)
                csr_mmio64_to_afu_readdata[40] <= 1'b1;
            end
          AFU_ID_L  : csr_mmio64_to_afu_readdata <= afu_id[63:0];
          AFU_ID_H  : csr_mmio64_to_afu_readdata <= afu_id[127:64];
          DFH_RSVD0 : csr_mmio64_to_afu_readdata <= '0;
          DFH_RSVD1 : csr_mmio64_to_afu_readdata <= '0;
          AFU_RESET : csr_mmio64_to_afu_readdata <= '0;
          AFU_IRQ_EN: csr_mmio64_to_afu_readdata <= '0;
          default: csr_mmio64_to_afu_readdata <= '0;
        endcase // csr_mmio64_to_afu_address

        if (reset_i) begin
            csr_mmio64_to_afu_readdatavalid <= 1'b0;
        end
    end : mmio_read

    // Write response
    // logic [$bits(csr_mmio64_to_afu_writeresponse     ) -1 : 0 ] writeresponse_q;
    // logic [$bits(csr_mmio64_to_afu_writeresponseuser ) -1 : 0 ] user_q;
    logic [$bits(csr_mmio64_to_afu_writeresponsevalid) -1 : 0 ] writeresponsevalid_q;

    // Output assignments
    assign enable_kernel_irq_o = enable_kernel_irq_q;
    assign reset_n_kernel_o    = reset_n_kernel_q;
    // assign csr_mmio64_to_afu_writeresponse      = writeresponse_q;
    // assign csr_mmio64_to_afu_writeresponseuser  = user_q;
    assign csr_mmio64_to_afu_writeresponsevalid = writeresponsevalid_q;

    always_comb begin : mmio_write
        // Default values
        reset_n_kernel_d    = 1'b1; // Don't keep state, just one cycle reset
        enable_kernel_irq_d = enable_kernel_irq_q;

        if ( is_csr_write ) begin : case_is_csr_write
            case ( csr_mmio64_to_afu_address ) // csr_mmio64_to_afu_address
                AFU_RESET   : begin
                    if ( csr_mmio64_to_afu_byteenable[0] ) begin
                        reset_n_kernel_d    = csr_mmio64_to_afu_writedata[0];
                    end
                end
                AFU_IRQ_EN  : begin
                    if ( csr_mmio64_to_afu_byteenable[0] ) begin
                        enable_kernel_irq_d = ~csr_mmio64_to_afu_writedata[0];
                    end
                end
                // write-ignore
                default     : $warning("This DFH CSR address (%d) is write-ignored", csr_mmio64_to_afu_address);
            endcase // csr_mmio64_to_afu_address
        end : case_is_csr_write
    end : mmio_write

    always_ff @(posedge clock_i) begin : regs
        // writeresponse_q <= '0;
        // user_q          <= csr_mmio64_to_afu_user;

        if (reset_i) begin
            reset_n_kernel_q     <= 1'b1;
            writeresponsevalid_q <= 1'b0;
            enable_kernel_irq_q  <= 1'b0;
        end
        else begin
            reset_n_kernel_q     <= reset_n_kernel_d;
            writeresponsevalid_q <= is_csr_write;
            enable_kernel_irq_q  <= enable_kernel_irq_d;
        end
    end : regs

endmodule : dfl_csr_avalon