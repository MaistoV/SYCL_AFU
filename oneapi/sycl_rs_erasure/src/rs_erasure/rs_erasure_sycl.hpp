#ifndef _RS_ERASURE_SYCL_H_
#define _RS_ERASURE_SYCL_H_

// SYCL header
#include <sycl/sycl.hpp>
#include <sycl/ext/intel/ac_types/ac_int.hpp>

// Reed-Solomon header
#include "rs_erasure.hpp"

/////////////
// Defines //
/////////////
// Limit the unroll for RS_K > LOOP_READ_CELLS_UNROLL_THRESHOLD
#define LOOP_READ_CELLS_UNROLL_THRESHOLD 6
#ifndef LOOP_READ_CELLS_UNROLL
  #if (RS_K > LOOP_READ_CELLS_UNROLL_THRESHOLD)
    #define LOOP_READ_CELLS_UNROLL LOOP_READ_CELLS_UNROLL_THRESHOLD
  #else // ! (RS_K > LOOP_READ_CELLS_UNROLL_THRESHOLD)
    #define LOOP_READ_CELLS_UNROLL RS_K
  #endif // ! (RS_K > LOOP_READ_CELLS_UNROLL_THRESHOLD)
#endif // ifndef LOOP_READ_CELLS_UNROLL

//////////////
// Typedefs //
//////////////

// Custom AC types
typedef ac_int<LINE_BIT_WIDTH, false> line_t;
typedef ac_int<1, false> uint1;
typedef ac_int<8, false> uint8;
typedef ac_int<16, false> uint16;

#ifndef NO_SYCL
  // Definitions for memory interfaces
  #define BUFFER_LOCATION_READ 1
  #define BUFFER_LOCATION_WRITE 2
  // Match Avalon MM hostchan parameters
  // see ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv
  #define	ADDR_WIDTH 	41              // This should match ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv
  #define	DATA_WIDTH 	LINE_BIT_WIDTH  // This should match ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv
  #define	ALIGN		    LINE_BYTE_WIDTH // This should match ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv
  #define MAX_BURST   64              // This should match ofs_plat_if_top_config.vh and ofs_plat_avalon_mem_rdwr_if.sv

  // Read interface
  typedef decltype(sycl::ext::oneapi::experimental::properties{
    sycl::ext::intel::experimental::buffer_location<BUFFER_LOCATION_READ>,
    sycl::ext::intel::experimental::awidth<ADDR_WIDTH>,
    sycl::ext::intel::experimental::dwidth<DATA_WIDTH>,
    sycl::ext::intel::experimental::latency<0>,
    sycl::ext::oneapi::experimental::alignment<ALIGN>,
    sycl::ext::intel::experimental::read_write_mode_read, // Read-only
    sycl::ext::intel::experimental::maxburst<MAX_BURST>
  }) read_properties;

  // Write interface
  typedef decltype(sycl::ext::oneapi::experimental::properties{
    sycl::ext::intel::experimental::buffer_location<BUFFER_LOCATION_WRITE>,
    sycl::ext::intel::experimental::awidth<ADDR_WIDTH>,
    sycl::ext::intel::experimental::dwidth<DATA_WIDTH>,
    sycl::ext::intel::experimental::latency<0>,
    sycl::ext::oneapi::experimental::alignment<ALIGN>,
    sycl::ext::intel::experimental::read_write_mode_write, // Write-only
    sycl::ext::intel::experimental::maxburst<MAX_BURST>
  }) write_properties;

  // Interface typedefs
  typedef sycl::ext::oneapi::experimental::annotated_arg<line_t *, read_properties > device_read_t;
  typedef sycl::ext::oneapi::experimental::annotated_arg<line_t *, write_properties> device_write_t;
#else // NO_SYCL
  // For BSP builds
  // Don't set any properties, no annotation, just pointers
  typedef line_t* device_read_t;
  typedef line_t* device_write_t;
#endif // NO_SYCL

//////////////////////////
// Invocation functions //
//////////////////////////

// Lambda
void RunKernelLambda(
                sycl::queue& q,
                unsigned int num_erasures,
                device_read_t buf_master_read,
                device_write_t buf_master_write,
                rs_erasure_csr_t csr
              );

// Function encapsulating the Reed-Solomon logic
void rs_erasure (
                device_read_t     master_read,
                device_write_t    master_write,
                rs_erasure_csr_t 	rs_erasure_csr
                );

#endif // _RS_ERASURE_SYCL_H_