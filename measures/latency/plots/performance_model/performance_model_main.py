#!/usr/local/bin/python
# Description: Use PerformanceModelc class to compute upper/lower bounds

import pandas
import performance_model_class

##################
# Fix parameters #
##################
# Assume RS[3:2]
K=3
P=2
# 1MB cell cell_len
cell_len=1024*1024

# Multi-threading factors
N_VFPClient     = 1
N_coder         = N_VFPClient       # Basic HDFS integration
N_PAC           = 1                 # Single-PAC
N_VF            = 1

# Constants
alpha_VFPClient = 1
alpha_VF        = 1
alpha_len       = 1
# alpha_MP        = 1
u_PCIe          = 32 * 1024 * 1024 * 1024 / K # 32 GB/s

# Latencies
path_to_csv = "empirical_overheads.csv"
emprical_data_overhead = pandas.read_csv(path_to_csv, sep=";")
cell_len_string = "1MB"
t_MP  = emprical_data_overhead.loc[
            emprical_data_overhead['cell_length'] == cell_len_string, 'MP-VFP'
            ].values[0]
# VFP overead
t_VFP = emprical_data_overhead.loc[
            emprical_data_overhead['cell_length'] == cell_len_string, 'VFP-AFU'
            ].values[0]
# SYCL AFU measures
t_AFU = emprical_data_overhead.loc[
            emprical_data_overhead['cell_length'] == cell_len_string, 'AFU'
            ].values[0]

# Statistically not different from each other (for the samples collected)
t_VFPClient = t_MP

# Components' upper bounds
SYCL_fmax_Hz = 350 * 10**6
SYCL_bytes_read_per_cycle = 64
SYCL_II = 2
# peak throughput = P * fmax * bytes_read_per_cycle / II
# 3:2 -> 2 * 350 * 10^6 * 64 / 2 = 21,875 GB/s
# 6:3 -> 3 * 350 * 10^6 * 64 / 2 = 32,8125 GB/s
u_RS        = P * SYCL_fmax_Hz * SYCL_bytes_read_per_cycle / SYCL_II
u_coder     = 1
# u_VFPClient = 1
# u_VFP       = 1
# u_AFU       = 1

model = performance_model_class.PerformanceModel (
    # RS[3:2]
    K                = K,
    P                = P,
    # Cell lenght
    cell_len         = cell_len,
    # Multi-threading factors
    N_VF             = N_VF,
    N_PAC            = N_PAC,
    N_coder          = N_coder,
    N_VFPClient      = N_VFPClient,
    # Constants
    alpha_VFPClient  = alpha_VFPClient,
    alpha_VF         = alpha_VF,
    alpha_len        = alpha_len,
    # alpha_MP         = alpha_MP, # Derived from alpha_VFPClient, alpha_VF, alpha_len
    u_PCIe           = u_PCIe,
    # Measured latencies
    t_MP             = t_MP,
    t_VFP            = t_VFP,
    t_AFU            = t_AFU,
    t_VFPClient      = t_VFPClient,
    # Measured throughput
    u_RS             = u_RS,
    # Derived thoughputs
    u_coder          = u_coder, # TBD
    # u_VFPClient      = u_VFPClient,
    # u_VFP            = u_VFP,
    # u_AFU            = u_AFU,
)


throughputs = model.get_all_throughputs()
for name, value in throughputs.items():
    print(f"{name}: {value}")