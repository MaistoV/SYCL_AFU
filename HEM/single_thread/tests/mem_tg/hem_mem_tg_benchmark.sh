#!/bin/bash

# Traffic Generator AFU Test Application
# Simulate RS traffic
# NOTE: this does not make sense for FPGA memory

# --loops UINT=1              Number of read/write loops to be run
# -w,--writes UINT=1          Number of unique write transactions per loop
# -r,--reads UINT=1           Number of unique read transactions per loop
# -b,--bls UINT=1             AXI4 burst length of each request. Supports 1-256 transfers beginning from 0. default: 0
# --stride UINT=1             Address stride for each sequential transaction
# -f,--mem-frequency UINT=300   Memory traffic clock frequency in MHz

# Parameters
K=6
P=3
_1MB=0x100000
max_axi_burst_len=256
num_axi_bursts_per_MB=$((1024*1024/$max_axi_burst_len)) # 4816

mem_tg --bls $(($max_axi_burst_len-1))                        \
        --loops  $(( $(($K + 1)) * $num_axi_bursts_per_MB  )) \
        --reads  $(($K * $num_axi_bursts_per_MB))             \
        --writes $((1 * $num_axi_bursts_per_MB))              \
        --stride $_1MB                                        \
        tg_test