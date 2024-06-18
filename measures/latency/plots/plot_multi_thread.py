import matplotlib.pyplot as plt
import glob
import pandas
import numpy
import sys
import os
import plot_common as common

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

##################
# Local override #
##################
hw_configs_local = ["" for _ in range(2) ]
hw_configs_local [common.ISA_L	 ] = common.hw_configs[common.ISA_L   ]
hw_configs_local [common.SYCL_AFU] = common.hw_configs[common.SYCL_AFU]
# hw_configs_local [common.PLAIN_C] = common.hw_configs[common.PLAIN_C_AFU]

###########################
# Source data directories #
###########################

data_dirs = ["" for _ in range(len(hw_configs_local)) ]
data_dirs[common.ISA_L   ]	= root_data_dir + "/data_ISA_L"
data_dirs[common.SYCL_AFU] = root_data_dir + "/data_SYCL_AFU"
# data_dirs[1] = root_data_dir + "/data_PLAIN_C"

#############
# Read data #
#############
	
# Per-PID data
throughput_B_s			= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]
latency_s 				= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]
# Cumulative
mean_throughput_B_s			= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]
mean_latency_s 				= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]
cumulative_throughput_B_s   = [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]
cumulative_latency_s		= [[[0. for _ in range(len(common.cell_length)) ] for _ in range(len(hw_configs_local))] for _ in range(len(common.RS_SCHEMA_list))]

# Loop over hw configs
num_threads = [0. for _ in range(len(hw_configs_local))]
for hw in range(0,len(hw_configs_local)):
	# Compose base directories: <data_dirs[hw]>_<pid>
	data_dirs_pid = glob.glob(data_dirs[hw] + "_*")
	# Count how many threads
	num_threads[hw] = len(data_dirs_pid)
	assert(num_threads[hw] != 0)

    # Loop over directories, RS, common.cell_length
	for dir_index in range(0,len(data_dirs_pid)):
		for rs in range(0,len(common.RS_SCHEMA_list)):
			for l in range(0,len(common.cell_length)):
				afu_latency_s = 0.
		
				# Compose filename
				afu_latency_s_file_name = data_dirs_pid[dir_index] + "/" + 'latency_' + common.RS_SCHEMA_list[rs] + "_" + common.cell_length[l] + "_" + hw_configs_local[hw] + '.txt'
				file_name_ref = glob.glob(afu_latency_s_file_name)
				if ( len(file_name_ref) != 1 ): 
					print("File name error: " + afu_latency_s_file_name)
					afu_latency_s = numpy.inf
					continue
				
				# Load data
				try:
					afu_latency_s = pandas.read_csv(file_name_ref[0], sep=";", header=None)
					# print(afu_latency_s)
				except:
					# print("File name error: " + afu_latency_s_file_name)
					afu_latency_s = numpy.inf

				# Save median for latency (seconds)
				latency_s[rs][hw][l] = numpy.median(afu_latency_s)

				# Save throughput (byte/second)
				throughput_B_s[rs][hw][l] = common.cell_length_int[l] / latency_s[rs][hw][l]
				# Multiply for P for multi-erasure reconstruction
				if os.environ['MULTI_ERASURE_SIMPLE'] == "1":
					throughput_B_s[rs][hw][l] *= common.RS_P_list[rs]
		
				# Accumulate throughput
				cumulative_throughput_B_s[rs][hw][l] += throughput_B_s[rs][hw][l]
				
				# Accumulate latency
				cumulative_latency_s[rs][hw][l] += latency_s[rs][hw][l]
				
# Compute averages
for hw in range(0,len(hw_configs_local)):
	for rs in range(0,len(common.RS_SCHEMA_list)):
		for l in range(0,len(common.cell_length)):
			# Average latency
			mean_latency_s     [rs][hw][l] = cumulative_latency_s     [rs][hw][l] / num_threads[hw]
			# Average throughput
			mean_throughput_B_s[rs][hw][l] = cumulative_throughput_B_s[rs][hw][l] / num_threads[hw]
	
##########################
# Figure average latency #
##########################
plt.figure("Multithreading Average Latency", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	for hw in range(0,len(hw_configs_local)):
		plt.loglog(
					common.cell_length_int, 
			 		mean_latency_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					linewidth=common.hw_linewidth[hw]
				)
	# Decorating
	plt.title("RS[" + common.RS_SCHEMA_txt[rs] + "]")
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.xlabel("Cell length")
	plt.ylabel("Seconds")
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45, minor=False)
	plt.grid(visible=True)
	plt.legend()
figname = plot_dir + "/" + "Multithreading_Average_Latency" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

#############################
# Figure average throughput #
#############################
plt.figure("Multithreading Average Throughput", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	plt.axhline(y=common.peak_phy_throughput[rs], linestyle='-' , color="r", linewidth=2, label="Max PCIe read bandwidth")
	plt.axhline(y=common.peak_asp_throughput[rs], linestyle='--', color="r", linewidth=2, label="Max ASP read bandwidth")
	for hw in range(0,len(hw_configs_local)):
		plt.loglog(
					common.cell_length_int, 
			 		mean_throughput_B_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					linewidth=common.hw_linewidth[hw]
				)
		# print(throughput_B_s[rs][hw][len(common.cell_length)-1]/GB)

	# Decoration
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.title("RS[" + common.RS_SCHEMA_txt[rs] + "]")
	plt.yticks(common.B_s_int, common.B_s)
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)
	plt.xlabel("Cell length")
	plt.ylabel("Throughput (B/s)")
	plt.legend()

figname = plot_dir + "/" + "Multithreading_Average_Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)

#################################
# Figure cumulative throughtput #
#################################
plt.figure("Multithreading Cumulative Throughput", figsize=[16,9])
ax = plt.subplot(1,2,1)
plt.tick_params(labelbottom=False, bottom=False)
for rs in range(0,len(common.RS_SCHEMA_list)):
	ax = plt.subplot(1,2,rs+1, sharey=ax)
	plt.axhline(y=common.peak_phy_throughput[rs], linestyle='-' , color="r", linewidth=2, label="Max PCIe read bandwidth")
	plt.axhline(y=common.peak_asp_throughput[rs], linestyle='--', color="r", linewidth=2, label="Max ASP read bandwidth")
	for hw in range(0,len(hw_configs_local)):
		plt.loglog(
					common.cell_length_int, 
			 		cumulative_throughput_B_s[rs][hw],
					common.hw_line[hw] + common.hw_marker[hw],
					label=common.hw_name[hw],
					linewidth=common.hw_linewidth[hw]
				)
		# print(throughput_B_s[rs][hw][len(common.cell_length)-1]/GB)

	# Decoration
	ax = plt.gca(); ax.set_xscale("log", base=2); ax.set_yscale("log", base=10)
	plt.axvline(x = common.MB, linestyle='--', color="k") # Vertical line at 1MB
	plt.grid(visible=True, which="both")
	plt.title("RS[" + common.RS_SCHEMA_txt[rs] + "]")
	plt.yticks(common.B_s_int, common.B_s)
	plt.xticks(common.cell_length_int, common.cell_length, rotation=45)
	plt.xlabel("Cell length")
	plt.ylabel("Throughput (B/s)")
	plt.legend()


figname = plot_dir + "/" + "Multithreading_Cumulative_Throughput" + ".png"
plt.savefig(figname, dpi=400, bbox_inches="tight")
print("Figure available at " + figname)
