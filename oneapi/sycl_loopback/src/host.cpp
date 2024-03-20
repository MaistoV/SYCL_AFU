//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================

#include <iostream>
#include <vector>

#include <sycl/sycl.hpp>
#include <sycl/ext/intel/fpga_extensions.hpp>

#include "exception_handler.hpp"

#include "kernel.hpp"

using namespace sycl;

int main() {
  // Select either the FPGA emulator, FPGA simulator or FPGA device
#if FPGA_SIMULATOR
  auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
  auto selector = sycl::ext::intel::fpga_selector_v;
#else  // #if FPGA_EMULATOR
  auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

  uint64_t length_lines = 2;

  // Interface arguments for kernel
  line_t* device_read  = (line_t*)malloc(sizeof(line_t) * length_lines);
  line_t* device_write = (line_t*)malloc(sizeof(line_t) * length_lines);

  // Check pointers are valid
  assert(device_read);
  assert(device_write);

  try {

    // Create a queue bound to the chosen device.
    // If the device is unavailable, a SYCL runtime exception is thrown.
    queue q(selector, fpga_tools::exception_handler);

    auto device = q.get_device();

    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl;

    // Writing on input buffer
    // "Hello world!" = 0x0021646c726f77206f6c6c6548
    ((uint64_t*)device_read)[0] = (uint64_t)0x6f77206f6c6c6548ul;
    ((uint64_t*)device_read)[1] = (uint64_t)0x0000000021646c72ul;
    
    printf("%s:%d: Host says: %s \n", __FILE__, __LINE__, (uint8_t*)device_read);
    printf("%s:%d: Buffer device_read: \n", __FILE__, __LINE__);
    for ( int i = 0; i < (sizeof(line_t) * length_lines); i++ ) {
      printf("%hhx ", ((uint8_t*)device_read)[i]);
    }
    printf("\n");

    // Init output buffer
    for ( int i = 0; i < (sizeof(line_t) / sizeof(uint32_t) * length_lines); i++ ) {
      ((uint32_t*)device_write)[i] = 0xdeadbeef;
    }
    printf("%s:%d: Buffer device_write: \n", __FILE__, __LINE__);
    for ( int i = 0; i < (sizeof(line_t) * length_lines); i++ ) {
      printf("%hhx ", ((uint8_t*)device_write)[i]);
    }
    printf("\n");

		// Run kernel
		RunKernelLambda(
						q,
						device_read,
						device_write,
            length_lines
					);

  } catch (exception const &e) {
    // Catches exceptions in the host code
    std::cerr << "Caught a SYCL host exception:\n" << e.what() << "\n";

    // Most likely the runtime couldn't find FPGA hardware!
    if (e.code().value() == CL_DEVICE_NOT_FOUND) {
      std::cerr << "If you are targeting an FPGA, please ensure that your "
                   "system has a correctly configured FPGA board.\n";
      std::cerr << "Run sys_check in the oneAPI root directory to verify.\n";
      std::cerr << "If you are targeting the FPGA emulator, compile with "
                   "-DFPGA_EMULATOR.\n";
    }
    std::terminate();
  }

  // At this point, the device buffers have gone out of scope and the kernel
  // has been synchronized. 
  // Print the string written by the FPGA
  printf("%s:%d: Device says: %s \n", __FILE__, __LINE__, (uint8_t*)device_write);
  // Print bytes written by the FPGA
  printf("%s:%d: Buffer device_write: \n", __FILE__, __LINE__);
  for ( int i = 0; i < (sizeof(line_t) * length_lines); i++ ) {
    printf("%hhx ", ((uint8_t*)device_write)[i]);
  }
  printf("\n");

  return 0;
}
