//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <sycl/ext/intel/fpga_extensions.hpp>

#include "kernel.hpp"

// Function implementing the kernel logic
void loopback ( 
      device_read_t   device_read,
      device_write_t  device_write,
      uint64_t        length_lines
     ) {
  
  // Match syntax from oneAPI-samples/DirectProgramming/C++SYCL_FPGA/ReferenceDesigns/niosv/kernels/simple_dma/build  
  for ( uint64_t i = 0; i < length_lines; i++ ) {
    // Mock write on the first line to test writes without a read
    if ( i == 0 ) {
      line_t word_to_write;
      // Write "Filling the first line!!..!!"
      ((uint64_t*)&word_to_write)[0] = (uint64_t)0x20676e696c6c6946u;
      ((uint64_t*)&word_to_write)[1] = (uint64_t)0x7372696620656874u;
      ((uint64_t*)&word_to_write)[2] = (uint64_t)0x2121656e696c2074u;
      ((uint64_t*)&word_to_write)[3] = (uint64_t)0x2121212121212121u;
      ((uint64_t*)&word_to_write)[4] = (uint64_t)0x2121212121212121u;
      ((uint64_t*)&word_to_write)[5] = (uint64_t)0x2121212121212121u;
      ((uint64_t*)&word_to_write)[6] = (uint64_t)0x2121212121212121u;
      ((uint64_t*)&word_to_write)[7] = (uint64_t)0x2121212121212121u;
      // Write whole world
      device_write[i] = word_to_write;
    }
    else {
      device_write[i] = device_read[i-1];
    }
  }
} // loopback

// Forward declare the kernel names in the global scope. This FPGA best practice
// reduces compiler name mangling in the optimization reports.
class SYCLLoopbackID;

// Lambda
void RunKernelLambda( sycl::queue& q,
                device_read_t  device_read,
                device_write_t device_write,
                uint64_t       length_lines
              ){

    // Submit the kernel
    q.submit([&](sycl::handler &h) {
      // Use kernel_args_restrict to specify that pointers do not alias.
      h.single_task<SYCLLoopbackID>([=]() [[intel::kernel_args_restrict]] {
          loopback( device_read, device_write, length_lines );
        });
    });

    // Wait for kernel to exit
	  q.wait();
}
