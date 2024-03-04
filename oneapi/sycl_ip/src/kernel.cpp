#include <sycl/ext/intel/fpga_extensions.hpp>

#include "kernel.hpp"

// typedef sycl::ext::oneapi::experimental::annotated_arg<line_t*, decltype( sycl::ext::oneapi::experimental::properties{
//     sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_READ>,   // Address space
//     sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
//     sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
//     sycl::ext::intel::experimental::read_write_mode_read,              // Write-only
//     sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
//     // maxburst<value>	// Maximum number of data transfers
//     }
//   >
// ) master_read_t;

// typedef sycl::ext::oneapi::experimental::annotated_arg<line_t*, decltype( sycl::ext::oneapi::experimental::properties{
//     sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_READ>,   // Address space
//     sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
//     sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
//     sycl::ext::intel::experimental::read_write_mode_write,              // Write-only
//     sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
//     // maxburst<value>	// Maximum number of data transfers
//     }
//   >
// ) master_write_t;


// void rs_erasure (
//                 master_read_t     master_read,
//                 master_write_t    master_write,
//                 rs_erasure_csr_t 	rs_erasure_csr 
//                 );

// // Forward declare the kernel names in the global scope. This FPGA best practice
// // reduces compiler name mangling in the optimization reports.
// class rs_erasure_id;

// // With Accessors
// // Lambda
// void RunKernelLambda( sycl::queue& q, 
                // sycl::buffer<line_t,1>& buf_master_read, 
                // sycl::buffer<line_t,1>& buf_master_write,
//                 rs_erasure_csr_t csr
//               ){
//     // submit the kernel
//     q.submit([&](sycl::handler &h) {
//       // Data accessors 
//       sycl::accessor master_read (buf_master_read , h, sycl::read_only );
//       sycl::accessor master_write(buf_master_write, h, sycl::write_only);

//       // Kernel executes with pipeline parallelism on the FPGA.
//       // Use kernel_args_restrict to specify that a, b, and r do not alias.
//       h.single_task<rs_erasure_id>([=]() [[intel::kernel_args_restrict]] {
//         rs_erasure(master_read, master_write, csr);
//       });
//     });
// }

void RunKernelFunctor( sycl::queue& q, 
                line_t* master_read, 
                line_t* master_write,
                rs_erasure_csr_t csr
              ){
    // Submit the kernel
    q.single_task( rs_erasure_functor{master_read, master_write, csr} )
      .wait();
}
