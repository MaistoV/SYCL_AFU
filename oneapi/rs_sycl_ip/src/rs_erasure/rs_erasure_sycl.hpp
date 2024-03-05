#ifndef _RS_ERASURE_SYCL_H_
#define _RS_ERASURE_SYCL_H_

// SYCL header
#include <sycl/sycl.hpp>
#include <sycl/ext/intel/ac_types/ac_int.hpp>

// Reed-Solomon header
#include "rs_erasure.hpp"

//////////////
// Typedefs //
//////////////

// Compatible interfaces for top-level component
// TODO: should it be compatible to CCI-P, TLP or PIM's host_chan?
#define ADDR_SPACE_READ 1
#define ADDR_SPACE_WRITE 2

// Custom AC types
typedef ac_int<DATA_BYTE_WIDTH*8, false> line_t;
typedef ac_int<1, false> uint1;
typedef ac_int<8, false> uint8;
typedef ac_int<16, false> uint16;

// Functor
// struct RSErasureFunctor {
//     // Interface properties
//     using master_read_prop = decltype( sycl::ext::oneapi::experimental::properties{
//         sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_READ>,   // Address space
//         sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
//         sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
//         sycl::ext::intel::experimental::read_write_mode_read,               // Read-only
//         sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
//         // maxburst<value>	// Maximum number of data transfers
//         }
//     );
//     using master_write_prop = decltype( sycl::ext::oneapi::experimental::properties{
//         sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_WRITE>,   // Address space
//         sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
//         sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
//         sycl::ext::intel::experimental::read_write_mode_write,              // Write-only
//         sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
//         // maxburst<value>	// Maximum number of data transfers
//         }
//     );

//     // Arguments
//     sycl::ext::oneapi::experimental::annotated_arg<line_t*, master_read_prop > master_read;
//     sycl::ext::oneapi::experimental::annotated_arg<line_t*, master_write_prop> master_write;
//     rs_erasure_csr_t rs_erasure_csr;

//     void rs_erasure (
//                     line_t* master_read,
//                     line_t* master_write,
//                     rs_erasure_csr_t rs_erasure_csr 
//                     ) const;
//     // Operator
//     void operator()() const 
//     {
//         // Call to function
//         rs_erasure(master_read, master_write, rs_erasure_csr);
        
//         // Mock logic
//         // for (int idx = 0; idx < 10; idx++) {
//         //     // Dummy logic here
//         //     master_write[idx] = master_read[idx] +1;
//         // }
//     }
// };


//////////////////////////
// Invocation functions //
//////////////////////////

// Lambda
void RunKernelLambda( 
                sycl::queue& q, 
                line_t* buf_master_read, 
                line_t* buf_master_write,
                rs_erasure_csr_t csr
              );

// Functor
void RunKernelFunctor ( 
                sycl::queue& q, 
                sycl::buffer<line_t,1>& buf_master_read, 
                sycl::buffer<line_t,1>& buf_master_write,
                rs_erasure_csr_t csr
              );

// Function encapsulating the Reed-Solomon logic              
void rs_erasure (
                line_t*     master_read,
                line_t*     master_write,
                rs_erasure_csr_t 	rs_erasure_csr 
                );

#endif // _RS_ERASURE_SYCL_H_