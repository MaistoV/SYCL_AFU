#ifndef _RS_ERASURE_SYCL_H_
#define _RS_ERASURE_SYCL_H_

// SYCL headers
#include <sycl/ext/intel/ac_types/ac_int.hpp>

// RS header
#include "rs_erasure.h"

// Compatible interfaces for top-level component
// TODO: should it be compatible to CCI-P, TLP or PIM's host_chan?
#define ADDR_SPACE_READ 1
#define ADDR_SPACE_WRITE 2
#define DATA_WITH 512
typedef ac_int<DATA_WITH, false> line_t;

struct rs_erasure_functor {
    // Interface properties
    using master_read_prop = decltype( sycl::ext::oneapi::experimental::properties{
        sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_READ>,   // Address space
        sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
        sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
        sycl::ext::intel::experimental::read_write_mode_read,               // Read-only
        sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
        // maxburst<value>	// Maximum number of data transfers
        }
    );
    using master_write_prop = decltype( sycl::ext::oneapi::experimental::properties{
        sycl::ext::intel::experimental::buffer_location<ADDR_SPACE_WRITE>,   // Address space
        sycl::ext::intel::experimental::dwidth<DATA_WITH>,                  // Data width
        sycl::ext::intel::experimental::latency<0>,                         // Minimum Latency 
        sycl::ext::intel::experimental::read_write_mode_write,              // Write-only
        sycl::ext::oneapi::experimental::alignment<DATA_WITH/8>             // Byte alignement
        // maxburst<value>	// Maximum number of data transfers
        }
    );

    // Arguments
    sycl::ext::oneapi::experimental::annotated_arg<line_t*, master_read_prop > master_read;
    sycl::ext::oneapi::experimental::annotated_arg<line_t*, master_write_prop> master_write;
    rs_erasure_csr_t rs_erasure_csr;

    // Operator
    void operator()() const {
        // Call to function
        // rs_erasure(master_read, master_write, rs_erasure_csr);
        for (int idx = 0; idx < 10; idx++) {
            // Dummy logic here
            master_write[idx] = master_read[idx] +1;
        }
    }
};


#endif // _RS_ERASURE_SYCL_H_