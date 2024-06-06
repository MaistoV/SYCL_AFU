import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os

# Utility Constants
KB = 1024
MB = 1024 * KB
GB = 1024 * MB

# Source data directory
root_data_dir = "../data/"
if len(sys.argv) >= 1:
	root_data_dir = sys.argv[1]

# Output directory for plots
plot_dir = "./output_plots"
if len(sys.argv) >= 2:
	plot_dir = sys.argv[2]

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

###############
# Set formats #
###############

# Hardware configurations
hw_configs = ["ISA-L", "SYCL_ASP", "SYCL_AFU" ] #, "PLAIN_C"]
ISA_L		= 0
SYCL_ASP  	= 1
SYCL_AFU  	= 2
# PLAIN_C		= 3

# Plot formats
hw_marker 		= ["" for _ in range(len(hw_configs)) ]
hw_line 		= ["" for _ in range(len(hw_configs)) ]
hw_linewidth	= ["" for _ in range(len(hw_configs)) ]
hw_color		= ["" for _ in range(len(hw_configs)) ]
hw_name			= ["" for _ in range(len(hw_configs)) ]

# ISA-L format
hw_marker		[ISA_L] = "*"
hw_line	 		[ISA_L] = "--"
hw_linewidth	[ISA_L] = 1
# hw_color		[ISA_L] = "b"
hw_name			[ISA_L] = "ISA-L"

# Plain C format
# hw_marker		[PLAIN_C] = "+"
# hw_line	 		[PLAIN_C] = "--"
# hw_linewidth	[PLAIN_C] = 1
# hw_name			[PLAIN_C] = "Plain C"

# SYCL ASP format
hw_marker		[SYCL_ASP] = "x"
hw_line			[SYCL_ASP] = "-"
hw_linewidth	[SYCL_ASP] = 1
# hw_color		[SYCL_ASP] = "r"
hw_name			[SYCL_ASP] = "SYCL ASP"

# SYCL AFU format
hw_marker		[SYCL_AFU] = "o"
hw_line	 		[SYCL_AFU] = "-"
hw_linewidth	[SYCL_AFU] = 2
# hw_color		[SYCL_AFU] = "g"
hw_name			[SYCL_AFU] = "SYCL AFU"


###########################
# Source data directories #
###########################
data_dirs 			= ["" for _ in range(len(hw_configs)) ]
data_dirs[ISA_L   ]	= root_data_dir + "/data_ISA_L/"
data_dirs[SYCL_ASP] = root_data_dir + "/data_SYCL_ASP_ASP_ZERO_COPY/"
data_dirs[SYCL_AFU] = root_data_dir + "/data_SYCL_AFU/"
# data_dirs[PLAIN_C ] = root_data_dir + "/data_PLAIN_C/"

########################
# Reed-Solomon formats #
########################
RS_SCHEMA_list = ["3_2", "6_3" ]
# RS_SCHEMA_list = ["3_2" ]
RS_SCHEMA_txt  = ["3:2", "6:3" ]
RS_color = ["g", "b"]
RS_6_3 = 1
RS_3_2 = 0
# Arrays of K:P values
RS_K_list = [3, 6]
RS_P_list = [2, 3]

###############
# Cell length #
###############
cell_length = [ 
				# "64B"	,	"128B",	"256B",	"512B", 
				"1KB"	,	"2KB"	,	"4KB"	,	"8KB"	,	"16KB"	,
				"32KB"	,	"64KB"	,	"128KB"	, 	"256KB"	,	"512KB"	,
				"1MB"	,	"2MB"	,	"4MB"	,	"8MB"	, 	"16MB"	,
				#  "32MB"	,	"64MB"#	,	"128MB"	, 	"256MB"	,	"512MB"	,
				]
cell_length_int = [ 
					# 64				, 128			, 256			, 512			, 
					1*KB	, 2*KB    , 4*KB    , 8*KB	 	, 16*KB 	,
					32*KB	, 64*KB   , 128*KB  , 256*KB	, 512*KB 	,
					1*MB    , 2*MB    , 4*MB    , 8*MB		, 16*MB 	,
					# 32*MB  , 64*MB   #, 128*MB  , 256*MB, 512*MB ,
				]
# index_1MB = cell_length.index("1MB")

#############
# Read data #
#############
# Preallocate arrays
mean_latency_s 		= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
# latency_s 			= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
throughput_B_s 		= [[[0. for _ in range(len(cell_length)) ] for _ in range(len(hw_configs))] for _ in range(len(RS_SCHEMA_list))]
afu_latency_s 		= [0. for _ in range(len(cell_length)) ]

# Figure Linear regression
for hw in range(0,len(hw_configs)):
	for rs in range(0,len(RS_SCHEMA_list)):
		for l in range(0,len(cell_length)):
			# Compose filename
			afu_latency_s_file_name = data_dirs[hw] + 'latency_' + RS_SCHEMA_list[rs] + "_" + cell_length[l] + "_" + hw_configs[hw] + '.txt'
			file_name_ref = glob.glob(afu_latency_s_file_name)
			if ( len(file_name_ref) != 1 ): 
				print("File name error: " + afu_latency_s_file_name)
				afu_latency_s = numpy.inf
				continue
			
			# Load data
			try:
				afu_latency_s = pandas.read_csv(file_name_ref[0], sep=";", header=None)
				# latency_s[rs][hw][l] = afu_latency_s
			except:
				print("File name error: " + afu_latency_s_file_name)
				afu_latency_s = numpy.inf

			# Save mean_latency_s
			# mean_latency_s[rs][hw][l] = numpy.average(afu_latency_s)
			mean_latency_s[rs][hw][l] = numpy.median(afu_latency_s)

			# Save throughput byte/second
			throughput_B_s[rs][hw][l] = cell_length_int[l] / mean_latency_s[rs][hw][l]
			# Multiply for P for multi-erasure reconstruction
			if os.environ['MULTI_ERASURE_SIMPLE'] == "1":
				throughput_B_s[rs][hw][l] *= RS_P_list[rs]

##################
# Figure Latency #
##################
plt.figure("Latency", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	for hw in range(0,len(hw_configs)):
		plt.loglog(
					cell_length_int, 
			 		mean_latency_s[rs][hw],
					hw_line[hw] + hw_marker[hw],
					label=hw_name[hw],
					linewidth=hw_linewidth[hw]
				)
	# Decorating
	plt.title("RS[" + RS_SCHEMA_txt[rs] + "]")
	plt.axvline(x = MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.xlabel("Cell length")
	plt.ylabel("Seconds")
	plt.xticks(cell_length_int, cell_length, rotation=45, minor=False)
	plt.grid(visible=True)
	plt.legend()
figname = plot_dir + "/" + "Latency" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

# Figure Throughput
B_s		= [ "100MB/s", "1GB/s", "10GB/s"]
B_s_int = [ 100*MB, 1*GB, 10*GB]
# Data for max physical throughput for PCIe Gen4 x16
PCIE_PHY_BANDWIDTH = 32 * GB # 32 GB/s
# Max read bandwidth as reported by "aocl diagnose acl0"
PCIE_ASP_BANDWIDTH = 16 * GB # 16 GB/s
# PAC is full duplex 32GB/s tx and 32G/s rx, tx and rx bandwidth should hinder each other, 
# therefore we just need to consider the worst case, which is reading the K input blocks
data_amount = numpy.array([ RS_K_list[RS_3_2], RS_K_list[RS_6_3] ])
# Derive arrays
peak_phy_throughput = PCIE_PHY_BANDWIDTH / data_amount
peak_asp_throughput = PCIE_ASP_BANDWIDTH / data_amount

plt.figure("Throughput", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	plt.axhline(y=peak_phy_throughput[rs], linestyle='-', color="r", linewidth=2, label="RS[" + RS_SCHEMA_txt[rs] + "] Max PCIe read bandwidth")
	plt.axhline(y=peak_asp_throughput[rs], linestyle='--', color="r", linewidth=2, label="RS[" + RS_SCHEMA_txt[rs] + "] Max ASP read bandwidth")
	for hw in range(0,len(hw_configs)):
		plt.loglog(
					cell_length_int, 
			 		throughput_B_s[rs][hw],
					hw_line[hw] + hw_marker[hw],
					label=hw_name[hw],
					linewidth=hw_linewidth[hw]
				)
		# print(throughput_B_s[rs][hw][len(cell_length)-1]/GB)

	# Decoration
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
	plt.axvline(x = MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.title("RS[" + RS_SCHEMA_txt[rs] + "]")
	plt.yticks(B_s_int, B_s)
	plt.xticks(cell_length_int, cell_length, rotation=45)
	plt.xlabel("Cell length")
	plt.ylabel("Throughput (B/s)")
	plt.legend()
figname = plot_dir + "/" + "Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

print("Plots are available at " + plot_dir)