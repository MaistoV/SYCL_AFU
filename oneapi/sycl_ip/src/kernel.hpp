//==============================================================
// Copyright Intel Corporation
//
// SPDX-License-Identifier: MIT
// =============================================================
#include <sycl/sycl.hpp>

// Reed-Solomon header
#include "rs_erasure.h"
// SYCL-related 
#include "rs_erasure_sycl.h"

void RunKernelLambda( sycl::queue& q, 
                line_t* buf_master_read, 
                line_t* buf_master_write,
                rs_erasure_csr_t csr
              );

void RunKernelFunctor( sycl::queue& q, 
                sycl::buffer<line_t,1>& buf_master_read, 
                sycl::buffer<line_t,1>& buf_master_write,
                rs_erasure_csr_t csr
              );