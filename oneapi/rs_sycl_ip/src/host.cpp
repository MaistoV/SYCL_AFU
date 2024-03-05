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

// Header for device code.
#include "rs_erasure_sycl.hpp"

using namespace sycl;

// the array size of vectors a, b and c
constexpr size_t kArraySize = 32;

int main() {
  std::vector<line_t> vec_a(kArraySize);
  std::vector<line_t> vec_b(kArraySize);

  // Fill vectors a and b with random float values
  for (size_t i = 0; i < kArraySize; i++) {
    vec_a[i] = rand() / (float)RAND_MAX;
    vec_b[i] = rand() / (float)RAND_MAX;
  }

  // Select either the FPGA emulator, FPGA simulator or FPGA device
#if FPGA_SIMULATOR
  auto selector = sycl::ext::intel::fpga_simulator_selector_v;
#elif FPGA_HARDWARE
  auto selector = sycl::ext::intel::fpga_selector_v;
#else  // #if FPGA_EMULATOR
  auto selector = sycl::ext::intel::fpga_emulator_selector_v;
#endif

  try {

    // Create a queue bound to the chosen device.
    // If the device is unavailable, a SYCL runtime exception is thrown.
    queue q(selector, fpga_tools::exception_handler);

    auto device = q.get_device();

    std::cout << "Running on device: "
              << device.get_info<sycl::info::device::name>().c_str()
              << std::endl;

    // Input argumens
    rs_erasure_csr_t rs_erasure_csr;
    rs_erasure_csr.erasure_pattern	= -1;
    rs_erasure_csr.survived_cells	= -1;
    rs_erasure_csr.cell_length_BYTE_WIDTH = 128u;

    // For Functor
    // Create the device buffers
    // buffer device_read (vec_a);
    // buffer device_write(vec_b);
    // RunKernelFunctor(q, device_read, device_write, rs_erasure_csr);
    
    // For Lambda
    line_t* device_read  = sycl::malloc_shared<line_t>(kArraySize, q);
    line_t* device_write = sycl::malloc_shared<line_t>(kArraySize, q);
    RunKernelLambda(q, device_read, device_write, rs_erasure_csr);

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
  // has been synchronized. Therefore, the output data (vec_r) has been updated
  // with the results of the kernel and is safely accesible by the host CPU.

  // Test the results
  size_t correct = 0;
  for (size_t i = 0; i < kArraySize; i++) {
    // float tmp = vec_a[i] + vec_b[i] - vec_r[i];
    // if (tmp * tmp < kTol * kTol) {
    //   correct++;
    // }
  }

  // Summarize results
  if (correct == kArraySize) {
    std::cout << "PASSED: results are correct\n";
  } else {
    std::cout << "FAILED: results are incorrect\n";
  }

  return !(correct == kArraySize);
}
