import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os

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
hw_configs = ["ISA-L", "SYCL_ASP", "SYCL_AFU", "PLAIN_C"]
ISA_L		= 0
PLAIN_C		= 1
SYCL_ASP  	= 2
SYCL_AFU  	= 3

# Plot formats
hw_marker 		= ["" for _ in range(len(hw_configs)) ]
hw_line 		= ["" for _ in range(len(hw_configs)) ]
hw_linewidth	= ["" for _ in range(len(hw_configs)) ]
hw_name			= ["" for _ in range(len(hw_configs)) ]

# ISA-L format
hw_marker		[ISA_L] = "*"
hw_line	 		[ISA_L] = "--"
hw_linewidth	[ISA_L] = 1
hw_name			[ISA_L] = "ISA-L"

# Plain C format
hw_marker		[PLAIN_C] = "+"
hw_line	 		[PLAIN_C] = "--"
hw_linewidth	[PLAIN_C] = 1
hw_name			[PLAIN_C] = "Plain C"

# SYCL ASP format
hw_marker		[SYCL_ASP] = "x"
hw_line			[SYCL_ASP] = "-"
hw_linewidth	[SYCL_ASP] = 1
hw_name			[SYCL_ASP] = "SYCL ASP"

# SYCL AFU format
hw_marker		[SYCL_AFU] = "o"
hw_line	 		[SYCL_AFU] = "-"
hw_linewidth	[SYCL_AFU] = 2
hw_name			[SYCL_AFU] = "SYCL AFU"


###########################
# Source data directories #
###########################
data_dirs 			= ["" for _ in range(len(hw_configs)) ]
data_dirs[ISA_L   ]	= root_data_dir + "/data_ISA_L/"
data_dirs[SYCL_ASP] = root_data_dir + "/data_SYCL_ASP/"
data_dirs[PLAIN_C ] = root_data_dir + "/data_PLAIN_C/"
data_dirs[SYCL_AFU] = root_data_dir + "/data_SYCL_AFU/"

########################
# Reed-Solomon formats #
########################
RS_SCHEMA_list = ["3_2", "6_3" ]
RS_SCHEMA_txt  = ["3:2", "6:3" ]
RS_color = ["r", "b"]
RS_6_3 = 1
RS_3_2 = 0
# Arrays of K:P values
RS_K_list = [3, 6]
RS_P_list = [2, 3]

###############
# Cell length #
###############
cell_length = [ 
				"64B"	,	"128B",	"256B",	"512B", 
				"1KB"	,	"2KB"	,	"4KB"	,	"8KB"	,	"16KB"	,
				"32KB"	,	"64KB"	,	"128KB"	, 	"256KB"	,	"512KB"	,
				"1MB"	,	"2MB"	,	"4MB"	,	"8MB"	, 	"16MB"	,
				 "32MB"	,	"64MB"#	,	"128MB"	, 	"256MB"	,	"512MB"	,
				]
cell_length_int = [ 64				, 128			, 256			, 512			, 
					1024           , 2*1024         , 4*1024         , 8*1024       , 16*1024 		,
					32*1024        , 64*1024        , 128*1024       , 256*1024     , 512*1024 		,
					1024*1024      , 2*1024*1024    , 4*1024*1024    , 8*1024*1024  , 16*1024*1024 	,
					32*1024*1024  , 64*1024*1024   #, 128*1024*1024  , 256*1024*1024, 512*1024*1024 ,
				]
index_1MB = cell_length.index("1MB")

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
			mean_latency_s[rs][hw][l] = numpy.average(afu_latency_s)
			# mean_latency_s[rs][hw][l] = numpy.median(afu_latency_s)

			# Save throughput byte/second
			throughput_B_s[rs][hw][l] = cell_length_int[l] / mean_latency_s[rs][hw][l]
			# Multiply for P for multi-erasure reconstruction
			if os.environ['MULTI_ERASURE_SIMPLE'] == "1":
				throughput_B_s[rs][hw][l] *= RS_P_list[rs]

##################
# Figure Latency #
##################
plt.figure("Latency", figsize=[16,9])
for hw in range(0,len(hw_configs)):
	for rs in range(0,len(RS_SCHEMA_list)):
		plt.loglog(
					cell_length_int, 
			 		mean_latency_s[rs][hw],
					RS_color[rs] + hw_line[hw] + hw_marker[hw],
					label="RS[" + RS_SCHEMA_txt[rs] + "] " + hw_name[hw],
					linewidth=hw_linewidth[hw]
				)
plt.axvline(x = 1024*1024, linestyle='--', color="g") # Vertical line at 1MB
plt.xlabel("Cell length")
plt.ylabel("seconds")
plt.xticks(cell_length_int, cell_length)
plt.grid(visible=True)
plt.legend()
figname = plot_dir + "/" + "Latency" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

# Figure Throughput
B_s		= [ "100MB/s", "1GB/s", "10GB/s"]
B_s_int = [ 100*1024*1024, 1024*1024*1024, 10*1024*1024*1024]
plt.figure("Throughput", figsize=[16,9])
for hw in range(0,len(hw_configs)):
	for rs in range(0,len(RS_SCHEMA_list)):
		plt.loglog(
					cell_length_int, 
			 		throughput_B_s[rs][hw],
					RS_color[rs] + hw_line[hw] + hw_marker[hw],
					label="RS[" + RS_SCHEMA_txt[rs] + "] " + hw_name[hw],
					linewidth=hw_linewidth[hw]
				)
		print(throughput_B_s[rs][hw][len(cell_length)-1]/1024/1024/1024)

ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)

plt.axvline(x = 1024*1024, linestyle='--', color="g") # Vertical line at 1MB
plt.grid(visible=True, which="both")
plt.yticks(B_s_int, B_s)
plt.xticks(cell_length_int, cell_length)
plt.xlabel("Cell length")
plt.ylabel("Throughput (B/s)")
plt.legend()
figname = plot_dir + "/" + "Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

print("Plots are available at " + plot_dir)