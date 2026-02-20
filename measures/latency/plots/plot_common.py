import numpy

# Common utilities and constants

################
# Figure sizes #
################
# 1 column
figsize_1column=[16,9]
# 2 columns
figsize_2columns=[16,4.5]
# Square
figsize_square=[16,16]

###############
# Set formats #
###############
# TODO: refactor this with dictionaries, e.g.:
# # Hardware configurations as a dictionary
# hw_configs = {
#     "ISA-L": {
#         "data_dir": "/data_ISA_L_0/",
#         "marker": "d",
#         "marker_fill": "full",
#         "line": "--",
#         "linewidth": 2,
#         "color": "purple",
#         "name": "ISA-L AVX-512"
#     },
#     "HWConfig.SYCL_ASP.value": {
#         "data_dir": "/data_SYCL_ASP_ASP_ZERO_COPY/",
#         "marker": "v",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 2,
#         "color": "g",
#         "name": "SYCL ASP"
#     },
#     "HWConfig.SYCL_AFU.value": {
#         "data_dir": "/data_SYCL_AFU/",
#         "marker": "o",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 2,
#         "color": "b",
#         "name": "SYCL AFU"
#     },
#     "SYCL_GPU": {
#         "data_dir": "/data_SYCL_GPU/",
#         "marker": "o",
#         "marker_fill": "none",
#         "line": "--",
#         "linewidth": 2,
#         "color": "c",
#         "name": "SYCL GPU"
#     },
#     "VFP_CLIENT": {
#         "data_dir": "/data_VFP_CLIENT/",
#         "marker": "o",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 2,
#         "color": "m",
#         "name": "VFP_CLIENT"
#     },
#     "HWConfig.MSG_PASSING.value": {
#         "data_dir": "/data_MSG_PASSING/",
#         "marker": "P",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 1,
#         "color": "r",
#         "name": "Message Passing"
#     },
#     "HWConfig.VFP.value": {
#         "data_dir": "/data_VFP/",
#         "marker": "X",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 2,
#         "color": "y",
#         "name": "VFProxy"
#     },
#     "HWConfig.DFS_BASIC.value": {
#         "data_dir": "",  # not defined in your code
#         "marker": "o",
#         "marker_fill": "full",
#         "line": "-",
#         "linewidth": 2,
#         "color": "k",
#         "name": "HDFS"
#     }
# }

# Hardware configurations
hw_configs = [
                "MSG_PASSING",
                "VFP",
              	"SYCL_AFU",
                # Stop here
                "VFP_CLIENT",
                "SYCL_ASP",
                "SYCL_GPU",
    			"ISA_L",
    			"DFS_BASIC",
			]
from enum import Enum, auto

HWConfig = Enum("HWConfig", {name: i for i, name in enumerate(hw_configs)})
hw_config_len = len(HWConfig)
# Only use first HW_CONFIG_MAX out of list
HW_CONFIG_MAX=3

# Source data directories
data_dirs = ["" for _ in range(hw_config_len) ]
data_dirs[HWConfig.ISA_L.value			] = "/data_ISA_L_0/"
data_dirs[HWConfig.SYCL_ASP.value		] = "/data_SYCL_ASP_ASP_ZERO_COPY/"
data_dirs[HWConfig.SYCL_AFU.value		] = "/data_SYCL_AFU/"
data_dirs[HWConfig.SYCL_GPU.value		] = "/data_SYCL_GPU/"
data_dirs[HWConfig.VFP_CLIENT.value		] = "/data_VFP_CLIENT/"
data_dirs[HWConfig.MSG_PASSING.value	] = "/data_MP/"
data_dirs[HWConfig.VFP.value			] = "/data_VFP/"
data_dirs[HWConfig.DFS_BASIC.value		] = "/data_DFS_BASIC/"

# Plot formats
hw_marker 		= ["o"    for _ in range(hw_config_len) ]
hw_marker_fill	= ["full" for _ in range(hw_config_len) ]
hw_line 		= ["-"    for _ in range(hw_config_len) ]
hw_linewidth	= [2      for _ in range(hw_config_len) ]
hw_color		= ["k"    for _ in range(hw_config_len) ]
hw_name			= ["" for _ in range(hw_config_len) ]

# ISA-L format
hw_marker		[HWConfig.ISA_L.value]= "d"
hw_marker_fill	[HWConfig.ISA_L.value]= "full"
hw_line	 		[HWConfig.ISA_L.value]= "--"
hw_linewidth	[HWConfig.ISA_L.value]= 2
hw_color		[HWConfig.ISA_L.value]= "purple"
hw_name			[HWConfig.ISA_L.value]= "ISA-L AVX-512"

# SYCL ASP format
hw_marker		[HWConfig.SYCL_ASP.value] = "v"
hw_marker_fill	[HWConfig.SYCL_ASP.value] = "full"
hw_line			[HWConfig.SYCL_ASP.value] = "-"
hw_linewidth	[HWConfig.SYCL_ASP.value] = 2
hw_color		[HWConfig.SYCL_ASP.value] = "g"
hw_name			[HWConfig.SYCL_ASP.value] = "SYCL ASP"

# SYCL AFU format
hw_marker		[HWConfig.SYCL_AFU.value] = "o"
hw_marker_fill	[HWConfig.SYCL_AFU.value] = "full"
hw_line	 		[HWConfig.SYCL_AFU.value] = "-"
hw_linewidth	[HWConfig.SYCL_AFU.value] = 2
hw_color		[HWConfig.SYCL_AFU.value] = "b"
hw_name			[HWConfig.SYCL_AFU.value] = "SYCL AFU"

# VFP_CLIENT format
hw_marker		[HWConfig.VFP_CLIENT.value] = "o"
hw_marker_fill	[HWConfig.VFP_CLIENT.value] = "full"
hw_line	 		[HWConfig.VFP_CLIENT.value] = "-"
hw_linewidth	[HWConfig.VFP_CLIENT.value] = 2
hw_color		[HWConfig.VFP_CLIENT.value] = "r"
hw_name			[HWConfig.VFP_CLIENT.value] = "VFPClient"

# Message Passing format
hw_marker		[HWConfig.MSG_PASSING.value] = "P"
hw_marker_fill	[HWConfig.MSG_PASSING.value] = "full"
hw_line	 		[HWConfig.MSG_PASSING.value] = "-"
hw_linewidth	[HWConfig.MSG_PASSING.value] = 2
hw_color		[HWConfig.MSG_PASSING.value] = "m"
hw_name			[HWConfig.MSG_PASSING.value] = "Message Passing"

# VFProxy format
hw_marker		[HWConfig.VFP.value] = "X"
hw_marker_fill	[HWConfig.VFP.value] = "full"
hw_line	 		[HWConfig.VFP.value] = "-"
hw_linewidth	[HWConfig.VFP.value] = 2
hw_color		[HWConfig.VFP.value] = "y"
hw_name			[HWConfig.VFP.value] = "VFProxy"

# VFProxy format
hw_marker		[HWConfig.DFS_BASIC.value] = "s"
hw_marker_fill	[HWConfig.DFS_BASIC.value] = "full"
hw_line	 		[HWConfig.DFS_BASIC.value] = "-"
hw_linewidth	[HWConfig.DFS_BASIC.value] = 2
hw_color		[HWConfig.DFS_BASIC.value] = "k"
hw_name			[HWConfig.DFS_BASIC.value] = "HDFS"

# SYCL GPU format
hw_marker		[HWConfig.SYCL_GPU.value] = "o"
hw_marker_fill	[HWConfig.SYCL_GPU.value] = "none"
hw_line	 		[HWConfig.SYCL_GPU.value] = "--"
hw_linewidth	[HWConfig.SYCL_GPU.value] = 2
hw_color		[HWConfig.SYCL_GPU.value] = "c"
hw_name			[HWConfig.SYCL_GPU.value] = "SYCL GPU"

########################
# Reed-Solomon formats #
########################
RS_SCHEMA_list = [
    				"3_2",
                   	# "6_3"
                   ]
RS_SCHEMA_txt  = ["RS[3:2]", "RS[6:3]" ]
RS_color = ["g", "b"]
RS_6_3 = 1
RS_3_2 = 0
# Arrays of K:P values
RS_K_list = [3, 6]
RS_P_list = [2, 3]

###############
# Data length #
###############

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

###############
# Cell length #
###############

cell_length = [
				# "64B"	,	"128B",	"256B",	"512B",
				# "1KB"	,	"2KB"	,	"4KB"	,	"8KB"	,	"16KB"	,
				# "32KB"	,	"64KB"	,	"128KB"	,
                "256KB"	,	"512KB"	,
				"1MB"	,	"2MB"	,	"4MB"	,	"8MB"	, #	"16MB"	,
				#  "32MB"	,	"64MB"	,	"128MB"	#, 	"256MB"	,	"512MB"	,
				]
cell_length_int = [
					# 64				, 128			, 256			, 512			,
					# 1*KB	, 2*KB    , 4*KB    , 8*KB	 	, 16*KB 	,
					# 32*KB	, 64*KB   , 128*KB  ,
                    256*KB	, 512*KB 	,
					1*MB    , 2*MB    , 4*MB    , 8*MB		,# 16*MB 	,
					# 32*MB  , 64*MB	  , 128*MB  #, 256*MB, 512*MB ,
				]
index_1MB = cell_length.index("1MB")

##################
# PCIe bandwidth #
##################

# Figure Throughput
B_s		= [ "10MB/s", "100MB/s", "1GB/s", "10GB/s"]
B_s_int = [ 10*MB, 100*MB, 1*GB, 10*GB]
# Data for max physical throughput for PCIe Gen4 x16
PCIE_PHY_BANDWIDTH = 32 * GB # 32 GB/s
# Max read bandwidth as reported by "aocl diagnose acl0"
PCIE_ASP_BANDWIDTH = 16 * GB # 16 GB/s
# PAC is full duplex 32GB/s tx and 32G/s rx, tx and rx bandwidth should hinder each other,
# therefore we just need to consider the worst case, which is reading the K input blocks
data_amount = [0. for _ in range(len(RS_SCHEMA_list))]
data_amount = numpy.array(data_amount)
data_amount [RS_3_2] = RS_K_list[RS_3_2]
# data_amount [RS_6_3] = RS_K_list[RS_6_3]

# Derive arrays
peak_phy_throughput = PCIE_PHY_BANDWIDTH / data_amount
peak_asp_throughput = PCIE_ASP_BANDWIDTH / data_amount