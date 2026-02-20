#!/usr/local/bin/python
import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os
import plot_common as common
from scipy.stats import trim_mean # for outliers

###############
# Directories #
###############
# Root
root_dir = "measures/latency/"

# Source data directory
root_data_dir = root_dir + "data_multi_erasure/"
if len(sys.argv) >= 2:
	root_data_dir = sys.argv[1] + "/"

# Output directory for plots
plot_dir = root_dir + "plots/output_plots_multi_erasure"
if len(sys.argv) >= 3:
	plot_dir = sys.argv[2]

# Create output directory
os.makedirs(plot_dir, exist_ok=True)

#############
# Read data #
#############
# Preallocate arrays
mean_latency_s 		= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(common.HW_CONFIG_MAX)] for _ in range(len(common.RS_SCHEMA_list))]
# latency_s 			= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(common.HW_CONFIG_MAX)] for _ in range(len(common.RS_SCHEMA_list))]
throughput_B_s 		= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(common.HW_CONFIG_MAX)] for _ in range(len(common.RS_SCHEMA_list))]
afu_latency_s 		= [0. for _ in range(len(common.cell_length)) ]

for hw in range(0,common.HW_CONFIG_MAX):
	for rs in range(0,len(common.RS_SCHEMA_list)):
		for l in range(0,len(common.cell_length)):
			# Compose filename, must match filenames from this project and VFProxy, e.g.:
			# - latency_3_2_1KB_SYCL_AFU
			# - latency_RS_3_2_numVFP1_numClients1_clientID0_1KB
			afu_latency_s_file_name = root_data_dir + common.data_dirs[hw] + 'latency_*' + common.RS_SCHEMA_list[rs] + "*_" + common.cell_length[l] + '*.txt'
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
			mean_latency_s[rs][hw][l] = trim_mean(afu_latency_s, 0.05) # Ditch low and high 10%s
			mean_latency_s[rs][hw][l] = numpy.average(afu_latency_s)
			# mean_latency_s[rs][hw][l] = numpy.median(afu_latency_s)

			# Adjust SYCL_GPU data
			if common.hw_configs[hw] == "SYCL_GPU":
			# 	mean_latency_s[rs][hw][l] /= 16
				# Adjust by Russian Peasant (RP) to ISA-L opts:
				#	int
				# 	Small tables (ST)
				# 	Large tables (LT)
				# from Chen et al. https://doi.org/10.1109/ASAP.2016.7760770
				# GB/s
				RP=0.4
				ST=0.9
				LT=1.8
				# RP to small tables
				# SYCL_GPU_ADJ = ST/RP # 0.9 / 0.4 = 2.25
				# RP to large tables
				SYCL_GPU_ADJ = LT/RP # 1.8 / 0.4 = 4.5
				# RP int to int64
				SYCL_GPU_ADJ *= 2
				# Account for a fully parallel mapping on Xe-HPG cores
				#	Use 16 cores as the comparable Flex 140 card
				SYCL_GPU_ADJ *= 16
				# Divide by adjustment factor
				mean_latency_s[rs][hw][l] /= SYCL_GPU_ADJ

			# Save throughput byte/second
			throughput_B_s[rs][hw][l] = common.cell_length_int[l] / mean_latency_s[rs][hw][l]
			# Multiply for P for multi-erasure reconstruction
			if os.environ['MULTI_ERASURE_SIMPLE'] == "1":
				throughput_B_s[rs][hw][l] *= common.RS_P_list[rs]

# Plot font size
plt.rcParams.update({'font.size': 18})

#############
# DFS BASIC #
#############

dfsbasic_data_dir = "measures/latency/data_multi_erasure/data_DFS_BASIC/"

nrFiles_list = [1, 2, 4, 6, 8]
dfsbasic_data = [0. for _ in range(len(nrFiles_list))]

for i, nrFiles in enumerate(nrFiles_list):
	# Compose file names: mix22222VFs_fileSize3MB_nrFiles${NR_FILES}_cellSize1024k
	filename = dfsbasic_data_dir + "mix22222VFs_fileSize3MB_nrFiles" + str(nrFiles) + "_cellSize1024k.csv"
	# Read data
	dfsbasic_data[i] = pandas.read_csv(filename, sep=";", header=None).mean().values[0]

# print(dfsbasic_data)

plt.figure("dfsbasic", figsize=[15,5])
plt.title("dfsbasic")
plt.plot(
	nrFiles_list,
	dfsbasic_data,
	"-o",
	markersize=10,
	linewidth=3,
)
# Decorate
plt.ylabel("Latency (s)")
plt.xlabel("Number of files")
plt.grid()
# Save
filename = "tmp.png"
plt.savefig(filename, dpi=400, bbox_inches="tight")
print("Figure available at " + filename)

#######################
# Compute differences #
#######################
MP=0
VFP_CLIENT=1
AFU=2
difference_labels = ["$u_{MP}$", "$u_{VFPClient}$" , "$u_{AFU}$"]

plt.figure("diff", figsize=[15,5])
# Only for 3:2
rs=common.RS_3_2
# For each hw config, skipping the first
diff = [[0. for _ in range(len(common.cell_length))] for _ in range(common.HW_CONFIG_MAX-1)]
for hw in range(0,common.HW_CONFIG_MAX-1):
	# for rs in range(0,len(common.RS_SCHEMA_list)):
	for l in range(0,len(common.cell_length)):
		# # Debug
		# print(common.hw_configs[hw], common.cell_length[l])
		# Compute difference between this latency and the next
		# NOTE: assuming they are ordered by latency (desceding)
		diff[hw][l] = mean_latency_s[rs][hw][l] - mean_latency_s[rs][hw+1][l]
		assert diff[hw][l] > 0, f"diff[{common.hw_configs[hw+1]}-{common.hw_configs[hw]}][{common.cell_length[l]}] = {diff[hw][l]}"
	# Debug
	# print(diff[hw])
	# Plot
	ax = plt.loglog(
	# ax = plt.plot(
		common.cell_length_int,
		diff[hw],
		"-o",
		markersize=10,
		linewidth=2,
		color=common.hw_color[hw],
		label=difference_labels[hw]
	)
	# Decorate
	plt.title(common.RS_SCHEMA_txt[rs])
# Plot SYCL AFU data as well
plt.loglog(
# plt.plot(
		common.cell_length_int,
		mean_latency_s[rs][common.HWConfig.SYCL_AFU.value],
		"-o",
		markersize=10,
		linewidth=2,
		color=common.hw_color[common.HWConfig.SYCL_AFU.value],
		label=difference_labels[-1]
)
# Decorate
plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
# DFSBASIC data
plt.plot(
	common.cell_length_int[common.index_1MB],
	dfsbasic_data[0],
	"*",
	markersize=20,
	color="k",
	label="$u_{HDFS}$"
)
plt.legend(loc="upper left")
plt.tick_params(which="minor", labelbottom=False, bottom=False)
plt.xticks(common.cell_length_int, common.cell_length, rotation=0)
plt.grid(visible=True, which="both")
plt.ylabel("Seconds (s)")
# Save
figname = plot_dir + "/" + "single_core_diff" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

# Plot throughput
plt.figure("tput", figsize=[15,10])
# Constant lines
plt.axhline(y=common.peak_phy_throughput[rs], linestyle='-' , color="r", linewidth=2, label="$U_{PCIe}$")
plt.axvline(x = common.MB, linestyle='-', color="k") # Vertical line at 1MB
# Only for 3:2
rs=common.RS_3_2
# Plot SYCL AFU data as well
plt.loglog(
# plt.plot(
		common.cell_length_int,
		throughput_B_s[rs][common.HWConfig.SYCL_AFU.value],
		"-o",
		markersize=10,
		linewidth=2,
		color=common.hw_color[common.HWConfig.SYCL_AFU.value],
		label=difference_labels[-1]
)
tput = [[0. for _ in range(len(common.cell_length))] for _ in range(common.HW_CONFIG_MAX-1)]
# For each hw config, skipping the first
for hw in reversed(range(0,common.HW_CONFIG_MAX-1)):
	for l in range(0,len(common.cell_length)):
		# Compute tput from difference
		tput[hw][l] = common.cell_length_int[l] / diff[hw][l]
	# Plot
	ax = plt.loglog(
	# ax = plt.plot(
		common.cell_length_int,
		tput[hw],
		"-",
		markersize=10,
		linewidth=2,
		marker=common.hw_marker[hw],
		color=common.hw_color[hw],
		label=difference_labels[hw]
	)
	# Decorate
	plt.title(common.RS_SCHEMA_txt[rs])
# DFSBASIC data
plt.plot(
	common.cell_length_int[common.index_1MB],
	common.cell_length_int[common.index_1MB] / dfsbasic_data[0],
	"*",
	markersize=20,
	color="k",
	label="$u_{HDFS}$"
)
# Plot multiplied data
N_VFPClients = 4
N_VF		 = 2
# U_{MP}
alpha_MP = 1e7
U_MP = N_VFPClients / N_VF * alpha_MP
plt.axhline(y=U_MP, linestyle='-' , color="m", linewidth=4, label="$U_{AFU}$")
# U_{VFPClient}
U_VFPClient = [ 0. for _ in range(len(common.cell_length))]
for l in range(0,len(common.cell_length)):
	U_VFPClient[l] = tput[VFP_CLIENT][l] * N_VFPClients
plt.plot(
		common.cell_length_int,
		U_VFPClient,
		label="$U_{VFPClient}$"
	)
# U_{AFU}
U_AFU = [ 0. for _ in range(len(common.cell_length))]
for l in range(0,len(common.cell_length)):
	U_AFU[l] = throughput_B_s[rs][common.HWConfig.SYCL_AFU.value][l] * N_VF # P is already multiplied in
# plt.plot(common.cell_length_int,U_AFU,label="$U_{AFU}$")
plt.axhline(y=U_AFU[0], linestyle='-' , color="b", linewidth=4, label="$U_{AFU}$")
# u_{RS}
## Components' upper bounds
SYCL_fmax_Hz = 350 * 10**6
SYCL_bytes_read_per_cycle = 64
SYCL_II = 2
P = 2 # RS[3:2]
## peak throughput = P * fmax * bytes_read_per_cycle / II
u_RS = P * SYCL_fmax_Hz * SYCL_bytes_read_per_cycle / SYCL_II
plt.axhline(y=u_RS, linestyle='-' , color="c", linewidth=2, label="$u_{RS}$")
# U_{RS}
U_RS = u_RS * N_VF
plt.axhline(y=U_RS, linestyle='-' , color="c", linewidth=4, label="$U_{RS}$")

# Decorate
plt.legend(loc="lower right")
plt.tick_params(which="minor", labelbottom=False, bottom=False)
plt.xticks(common.cell_length_int, common.cell_length, rotation=0)
plt.grid(visible=True, which="both", axis="x")
plt.ylabel("Throughput (B/s)")
# Save
figname = plot_dir + "/" + "single_core_diff_thoughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)


########################
# Model implementation #
########################
combined_dict = {
	"MP" : diff[0], # "MP-VFP
	"VFP" : diff[1], # "VFP-AFU
	"AFU" : mean_latency_s[rs][common.HWConfig.SYCL_AFU.value], # "SYCL AFU"
}

combined_df = pandas.DataFrame(combined_dict, index=common.cell_length)
combined_df.index.name = 'cell_length'

path_to_csv = "empirical_overheads.csv"
combined_df.to_csv(path_to_csv, sep=";")

# Debug
# print(combined_df)
